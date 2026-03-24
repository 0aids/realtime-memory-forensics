#include "logger.hpp"
#include "memory_graph.hpp"
#include "operations.hpp"
#include "types.hpp"
#include "utils.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <print>
#include <ranges>
#include <rmf.hpp>
#include <iostream>
#include <sched.h>
#include <chrono>
#include <map>
#include <thread>
#include <unordered_set>

using namespace rmf;
using namespace rmf::types;
using namespace std;

struct SourceTargetPointerPair
{
    uintptr_t source;
    uintptr_t target;
};
struct MapifiedSnap
{
    vector<SourceTargetPointerPair> sourceTargetPairs;
    MemoryRegionProperties          sourceMrp;
};

MapifiedSnap mapifySnap(const MemorySnapshot& snap)
{
    MapifiedSnap pointers;
    pointers.sourceMrp = snap.getMrp();

    const auto mrp  = snap.getMrp();
    const auto data = snap.getDataSpan();

    ptrdiff_t  head =
        sizeof(uintptr_t) - mrp.TrueAddress() % sizeof(uintptr_t);

    pointers.sourceTargetPairs.reserve(
        (mrp.relativeRegionSize - head) / sizeof(uintptr_t));

    const uintptr_t* ptr_data =
        reinterpret_cast<const uintptr_t*>(data.data() + head);
    size_t count =
        (mrp.relativeRegionSize - head) / sizeof(uintptr_t);

    for (size_t i = 0; i < count; ++i)
    {
        uintptr_t target = ptr_data[i];
        uintptr_t source =
            head + (i * sizeof(uintptr_t)) + mrp.TrueAddress();
        pointers.sourceTargetPairs.emplace_back(source, target);
    }

    // Sort the flat array by target address for fast binary searching later
    std::sort(pointers.sourceTargetPairs.begin(),
              pointers.sourceTargetPairs.end(),
              [](const auto& a, const auto& b)
              { return a.target < b.target; });

    return pointers;
}

// Search if a regions' true address lies within some other region.
vector<pair<MemoryRegionProperties, MemoryRegionProperties>>
findSourceTargetRegionsFast(const MapifiedSnap&              mapsnap,
                            const MemoryRegionPropertiesVec& regions,
                            const MrpRestructure& mrpRestructure)
{
    vector<pair<MemoryRegionProperties, MemoryRegionProperties>>
                result;
    const auto& sourceTargetPairs = mapsnap.sourceTargetPairs;

    for (auto target : regions)
    {
        target = utils::RestructureMrp(target, mrpRestructure);

        // Binary search the flat vector by the target
        auto sourcesBottom = std::lower_bound(
            sourceTargetPairs.begin(), sourceTargetPairs.end(),
            target.TrueAddress(),
            [](const SourceTargetPointerPair& element, uintptr_t val)
            { return element.target < val; });

        auto sourcesTop = std::upper_bound(
            sourceTargetPairs.begin(), sourceTargetPairs.end(),
            target.TrueEnd() - 1,
            [](uintptr_t val, const SourceTargetPointerPair& element)
            { return element.target > val; });

        for (auto it = sourcesBottom; it != sourcesTop; ++it)
        {
            MemoryRegionProperties newSourceMrp = mapsnap.sourceMrp;
            newSourceMrp.relativeRegionAddress =
                it->source - newSourceMrp.parentRegionAddress;
            newSourceMrp.relativeRegionSize = sizeof(uintptr_t);
            result.emplace_back(newSourceMrp, target);
        }
    }
    return result;
}

bool operator==(const rmf::types::MemoryRegionProperties& a,
                const rmf::types::MemoryRegionProperties& b)
{
    return a.TrueAddress() == b.TrueAddress() &&
        a.TrueEnd() == b.TrueEnd();
}
bool operator==(const rmf::types::MemoryRegionProperties a,
                const rmf::types::MemoryRegionProperties b)
{
    return a.TrueAddress() == b.TrueAddress() &&
        a.TrueEnd() == b.TrueEnd();
}

#pragma GCC push_options
#pragma GCC optimize("O0")
int         main(int argc, const char** argv)
{
    if (argc != 3)
    {
        cout << "Incorrect arguments! Expecting PID as second "
                "argument AND string to match for as third"
             << endl;
    }
    const MrpRestructure restructure{-32, 64};
    const pid_t          pid         = std::stoul(argv[1]);
    std::string          matchString = argv[2];

    rmf::g_logLevel = rmf_Info;
    graph::MemoryGraphData mg;

    auto                   og = rmf::utils::getMapsFromPid(pid)
                  .FilterHasPerms("r")
                  .FilterActiveRegions(pid);
    auto          sources = og;
    rmf::Analyzer analyzer(std::thread::hardware_concurrency());
    auto snaps = analyzer.Execute(rmf::types::MemorySnapshot::Make,
                                  sources, pid);
    {
        sources =
            analyzer.Execute(rmf::op::findString, snaps, matchString)
                .flatten();
        cout << format("num Results: {}", sources.size()) << endl;
    }
    MemoryRegionPropertiesVec targets;
    sources =
        analyzer.Execute(utils::RestructureMrp, sources, restructure);
    for (const auto& res : sources)
    {
        mg.addNode({res, 0});
    }
    size_t bfsDepth = 0;
    println("Initial Num Nodes: {}, Initial Num links: {}\n",
            mg.getNodes().size(), mg.getLinks().size());
    while (1)
    {
        println("Getting mapsSnaps!");
        og = rmf::utils::getMapsFromPid(pid)
                 .FilterHasPerms("r")
                 .FilterActiveRegions(pid);
        snaps = analyzer.Execute(rmf::types::MemorySnapshot::Make, og,
                                 pid);
        auto mapsSnaps = analyzer.Execute(mapifySnap, snaps);
        println("Running loop");
        size_t i = 0;
        while (sources.size() > 0 && i < 5)
        {
            auto newResult =
                analyzer
                    .Execute(findSourceTargetRegionsFast, mapsSnaps,
                             sources, MrpRestructure{0, 0})
                    .flatten();
            println("@ bfs{{{}}} new sources with duplicates: {}",
                    bfsDepth++, newResult.size());
            int numOldNodes = mg.getNodeCount();
            int numOldLinks = mg.getLinkCount();
            // lastNodeKeys represents the last sources added. Our new sources point to
            // the nodes represented in the last round, and our targets
            // represent our old sources..
            // This is pretty fucking slow...
            sources.clear();
            for (auto& [source, target] : newResult)
            {
                auto targetNodeKey = mg.getNodeKeyAtMrp(target);
                if (!targetNodeKey.has_value())
                    continue;
                auto old = source;

                source = utils::RestructureMrp(source, restructure);
                // Also make sure that the source doesn't already exist.
                if (!mg.containsMrp(source))
                    sources.push_back(source);
                auto sourceKey = mg.addNode({source, 0});

                graph::MemoryLinkData newData = {
                    .sourceMemberId = 0,
                    .targetMemberId = 0,
                    .sourceAddr     = old.TrueAddress(),
                    .targetAddr     = target.TrueAddress(),
                };
                mg.addLink(sourceKey, targetNodeKey.value(), newData);
            }
            println("@ bfs{{{}}} new sources without duplicates: {}",
                    bfsDepth - 1, sources.size());
            println("Num Nodes: {}, Num links: {}",
                    mg.getNodes().size(), mg.getLinks().size());
            println("diff Nodes: {}, Num links: {}\n",
                    mg.getNodes().size() - numOldNodes,
                    mg.getLinks().size() - numOldLinks);
            i++;
        }
        int numOldNodes = mg.getNodeCount();
        int numOldLinks = mg.getLinkCount();
        // Remove all keys and links that no longer exist.
        vector<graph::NodeKey> nodesToRemove;
        for (const auto [key, node] : mg.getNodes())
        {
            auto outgoingLinks = mg.getOutgoingLinks(key);
            auto snap = MemorySnapshot::Make(node.nodeData.mrp, pid);
            for (const auto& l : outgoingLinks)
            {
                if (!mg.getNodes().contains(l))
                    continue;
                if (op::findPointersToRegion(
                        snap, mg.getNode(l).value().nodeData.mrp)
                        .empty())
                {
                    nodesToRemove.push_back(key);
                    break;
                }
            }
        }
        for (const auto& key : nodesToRemove)
        {
            mg.removeNode(key);
        }
        mg.pruneDeadLinks();
        mg.pruneDeadNodes();
        println("Pruned graph for dead nodes");
        println("Num Nodes: {}, Num links: {}", mg.getNodes().size(),
                mg.getLinks().size());
        println("diff Nodes: {}, diff Num links: {}\n",
                (int)mg.getNodes().size() - numOldNodes,
                (int)mg.getLinks().size() - numOldLinks);
        sources.clear();
        for (const auto& [key, node] : mg.getNodes())
        {
            sources.push_back(node.nodeData.mrp);
        }
        println("Continue? press q to stop");
        char a;
        cin >> a;
        if (a == 'q')
            break;
        println("Continuing");
    }
    while (1)
    {
        this_thread::sleep_for(50ms);
    }
}
#pragma GCC pop_options
