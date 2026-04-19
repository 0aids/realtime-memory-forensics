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
    const MrpRestructure restructure{-64, 128};
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
        auto mapsSnaps = analyzer.Execute(op::mapifySnap, snaps);
        println("Running loop");
        size_t i = 0;
        while (sources.size() > 0 && i < 5)
        {
            auto newResult =
                analyzer
                    .Execute(op::findSourcesOfTargetRegions,
                             mapsSnaps, sources, MrpRestructure{0, 0})
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
        // This will be added to the MemoryGraph wrapper class.
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
        mg.pruneStaleLinks();
        mg.pruneStaleNodes();
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
    // --- REPL Phase ---
    println("\n=== Entering MRP Hex Inspector REPL ===");
    println("Available Nodes:");
    size_t                      ind = 0;
    vector<rmf::utils::SlotKey> nodeKeys;
    for (const auto& [key, node] : mg.getNodes())
    {
        // Adjust the formatting below depending on the exact underlying type of graph::NodeKey
        println("Option [{}] -  Node Key: ind - {}, gen - {}, "
                "Address: 0x{:x}",
                ind++, key.index, key.generation,
                node.nodeData.mrp.TrueAddress());
        nodeKeys.push_back(key);
    }

    while (true)
    {
        size_t ind = 0;
        for (const auto& [key, node] : mg.getNodes())
        {
            // Adjust the formatting below depending on the exact underlying type of graph::NodeKey
            println("Option [{}] -  Node Key: ind - {}, gen - {}, "
                    "Address: 0x{:x}",
                    ind++, key.index, key.generation,
                    node.nodeData.mrp.TrueAddress());
        }
        println("\nCommands: [q]uit, [r]estructure and dump");
        std::string cmd;
        cin >> cmd;
        if (cmd == "q" || cmd == "quit")
            break;

        if (cmd == "r")
        {
            int64_t   index;
            ptrdiff_t offset;
            ptrdiff_t
                size_delta; // Assuming MrpRestructure takes ptrdiff_t/size_t

            cout << "Enter index: ";
            if (!(cin >> index))
                break;

            // Attempt to fetch the node
            if (index >= (int64_t)nodeKeys.size() || index < 0)
            {
                println("index not found. Try again.");
                continue;
            }
            auto nodeOpt = mg.getNode(nodeKeys.at(index));
            if (!nodeOpt.has_value())
            {
                println("Node key not found. Try again.");
                continue;
            }

            cout << "Enter offset (e.g., -32, 0, 16): ";
            cin >> offset;
            cout << "Enter size delta (e.g., 64, 128): ";
            cin >> size_delta;

            // Restructure the MRP
            auto newMrp = utils::RestructureMrp(
                nodeOpt.value().nodeData.mrp,
                MrpRestructure{offset, size_delta});

            // Generate snapshot
            auto      snap     = MemorySnapshot::Make(newMrp, pid);
            auto      data     = snap.getDataSpan();
            uintptr_t baseAddr = newMrp.TrueAddress();

            if (data.empty())
            {
                println("Warning: Snapshot returned empty data.");
                continue;
            }

            // Print Hex Dump
            println("\nHex Dump for Address 0x{:x} (Size: {} bytes):",
                    baseAddr, data.size());
            for (size_t i = 0; i < data.size(); i += 16)
            {
                // Print Address
                cout << std::hex << std::setfill('0')
                     << std::setw(sizeof(uintptr_t) * 2)
                     << baseAddr + i << ": ";

                // Print Hex Bytes
                for (size_t j = 0; j < 16; ++j)
                {
                    if (i + j < data.size())
                    {
                        cout << std::hex << std::setw(2)
                             << std::setfill('0')
                             << static_cast<unsigned int>(
                                    static_cast<unsigned char>(
                                        data[i + j]))
                             << " ";
                    }
                    else
                    {
                        cout << "   "; // Pad missing bytes
                    }
                }

                cout << " | ";

                // Print ASCII Chars
                for (size_t j = 0; j < 16; ++j)
                {
                    if (i + j < data.size())
                    {
                        unsigned char c = data[i + j];
                        cout << (std::isprint(c) ?
                                     static_cast<char>(c) :
                                     '.');
                    }
                }
                cout << " |\n";
            }
            // Reset cout formatting back to default (decimal)
            cout << std::dec;
        }
    }

    return 0;
}
#pragma GCC pop_options
