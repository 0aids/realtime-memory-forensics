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
int main(int argc, const char** argv)
{
    if (argc != 3)
    {
        cout << "Incorrect arguments! Expecting PID as second "
                "argument AND string to match for as third"
             << endl;
    }
    const pid_t pid         = std::stoul(argv[1]);
    std::string matchString = argv[2];

    rmf::g_logLevel = rmf_Verbose;
    rmf::Analyzer analyzer(std::thread::hardware_concurrency() - 1);
    graph::MemoryGraph mg(analyzer, pid);
    auto fatPointerId = mg->structRegistry.registerr("fatPointer")
                            .field("void*", "pointer")
                            .end();

    const std::string matchStringType =
        std::format("char[{}]", matchString.size());
    mg->structRegistry.registerr("charString")
        .field(matchStringType, "chars")
        .end();

    auto og = rmf::utils::getMapsFromPid(pid)
                  .FilterHasPerms("r")
                  .FilterActiveRegions(pid);
    auto sources = og;
    auto snaps   = analyzer.Execute(rmf::types::MemorySnapshot::Make,
                                    sources, pid);
    // Find the initial nodes
    auto foundStrings =
        analyzer.Execute(op::findString, snaps, matchString)
            .flatten();
    std::vector<graph::NodeKey> mostRecentSources;
    for (const auto& found : foundStrings)
    {
        if (auto key =
                mg->addStructuredNode(found, "charString", "chars");
            key.has_value())
            mostRecentSources.push_back(key.value());
    }

    auto pointerId =
        *mg->structRegistry.getFieldOfParent(fatPointerId, "pointer");
    int oldLinks = mg->getLinkCount();
    int oldNodes = mg->getNodeCount();
    println("Starting source count: {}, Starting link count: {}",
            oldNodes, oldLinks);
    while (oldNodes > 0)
    {
        mostRecentSources = mg.findSourcesOfTargetsRelaxed(
            snaps, mostRecentSources, pointerId);
        int newLinks = mg->getLinkCount();
        int newNodes = mg->getNodeCount();
        println("Number of new sources: {}, Number of new links: {}",
                newNodes - oldNodes, newLinks - oldLinks);
        println("Current source count: {}, Current link count: {}",
                newNodes, newLinks);
        oldLinks = newLinks;
        oldNodes = newNodes;
        char a;
        cout << "Input q to quit, anything else to continue: "
             << flush;
        cin >> a;
        switch (a)
        {
            case 's':
                // Strict prune
                mg.strictPrune();
                println("After strict pruning - Current source "
                        "count: {}, Current link count: {}",
                        mg->getNodeCount(), mg->getLinkCount());
                sources = utils::getMapsFromPid(pid)
                              .FilterHasPerms("r")
                              .FilterActiveRegions(pid);
                snaps = analyzer.Execute(
                    rmf::types::MemorySnapshot::Make, sources, pid);
                break;
            case 'r':
                mg.relaxedPrune();
                println("After relaxed pruning - Current source "
                        "count: {}, Current link count: {}",
                        mg->getNodeCount(), mg->getLinkCount());
                sources = utils::getMapsFromPid(pid)
                              .FilterHasPerms("r")
                              .FilterActiveRegions(pid);
                snaps = analyzer.Execute(
                    rmf::types::MemorySnapshot::Make, sources, pid);
                break;
            case 'q': exit(0); break;
            default: break;
        }
    }
}
