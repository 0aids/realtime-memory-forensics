#include <gtest/gtest.h>
#include <memory>
#include <print>
#include <thread>
#include "logger.hpp"
#include "memory_graph.hpp"
#include "operations.hpp"
#include "rmf.hpp"
#include "utils.hpp"
#include "test_helpers.hpp"
#include "types.hpp"

using namespace rmf::graph;
using namespace rmf::types;
using namespace rmf::op;
using namespace rmf::test;
using namespace std;

TEST(TestProcessTest, SListComponent)
{
    // Assertions are done on the slist component side.
    testProcess tp;
    tp.build<SListComponent<uint32_t>>(
        std::vector<uint32_t>{1, 2, 3, 4, 5});
    tp.run();
    std::this_thread::sleep_for(1s);
    tp.stop();
}
struct Node
{
    uint32_t data;
    Node*    next;
};

// Can't figure out how to align shit.
// TEST(memoryGraphWrapperTest, LinkedListSearchFakeStack)
// {
//     rmf::g_logLevel = rmf_Verbose;
//     rmf::Analyzer        a(std::thread::hardware_concurrency() / 2);
//     std::vector<uint8_t> stack_unaligned(1024, 0);
//     EXPECT_EQ(stack_unaligned.size(), 1024);
//     std::span<uint8_t> stack(stack_unaligned.begin() + (sizeof(void*) - (ptrdiff_t)stack_unaligned.data() % sizeof(void*)), stack_unaligned.end());
//     // Only count the aligned part, create a span
//     MemoryRegionProperties fakeMrp = {
//         .parentRegionAddress = 0,
//         .parentRegionSize = 1024,
//         .relativeRegionAddress = 0,
//         .relativeRegionSize = 1024,
//         .regionName_sp = make_shared<string>("hello"),
//         .perms = rmf::types::Perms::Read | rmf::types::Perms::Write,
//     };
//     auto          vec = std::vector<uint32_t>{9898, 1212, 5555, 3232};
//     const uint8_t* start = stack.data();
//     // align the head.
//     start += sizeof(void*) - ((uintptr_t) start % sizeof(void*));
//     auto head = start;
//     decltype(head) last = 0;
//     for (const auto a : vec)
//     {
//         Node n = {
//             .data = a,
//             .next = last ? (Node*)last : nullptr,
//         };
//         memcpy((void*)head, &n, sizeof(n));
//         last = head;
//         head += sizeof(n) + 0x10;
//     }
//     MemoryGraph mg(a, -1);
//     const auto NodeId = mg->structRegistry.registerr("Node")
//         .field("uint32_t", "data")
//         .field("Node*", "next")
//         .end();
//     const auto snap = MemorySnapshot(fakeMrp, stack);
//     auto res = findNumeralExact<uint32_t>(snap, 1212);
// }

// Test for linked lists
TEST(memoryGraphWrapperTest, LinkedListSearch)
{
    rmf::g_logLevel = rmf_Verbose;
    rmf::Analyzer a(std::thread::hardware_concurrency() / 2);
    testProcess   tp;
    auto vec = std::vector<uint32_t>{0xfafafafa, 0xbababa, 0x9a9a9a9a,
                                     0xbabbabb};
    tp.build<SListComponent<uint32_t>>(vec);
    pid_t pid = tp.run();
    // We should be able to find these integers in memory easily?
    MemoryGraph mg(a, pid);
    const auto  NodeId = mg->structRegistry.registerr("Node")
                            .field("uint32_t", "data")
                            .field("Node*", "next")
                            .end();

    const auto dataId =
        mg->structRegistry.getFieldOfParent(NodeId, "data").value();
    const auto nodePointerId =
        mg->structRegistry.getFieldOfParent(NodeId, "next").value();

    auto maps         = rmf::utils::getMapsFromPid(pid);
    auto filteredMaps = maps; //.FilterActiveRegions(pid);
    // Find our initial nodes
    auto snaps = a.Execute(rmf::types::MemorySnapshot::Make,
                           filteredMaps, pid);
    // Try with the last node.
    auto foundLast =
        a.Execute(findNumeralExact<uint32_t>, snaps, vec[1])
            .flatten();
    EXPECT_GE(foundLast.size(), 0);
    println("Number of {} found: {}", vec[1], foundLast.size());
    // push them onto our mg
    std::vector<rmf::graph::NodeKey> mostRecentSources;
    for (const auto& found : foundLast)
    {
        // Unaligned versions would find that their members arent properly aligned.
        // if (found.TrueAddress() % 8 != 0) continue;
        mostRecentSources.push_back(
            mg->addStructuredNode(found, "Node", "data").value());
    }
    while (mostRecentSources.size() > 0)
    {
        EXPECT_GT(mg->getNodeCount(), 0);
        println("Before finding sources - Number links: {} Number "
                "Nodes: {}",
                mg->getLinkCount(), mg->getNodeCount());
        mostRecentSources = mg.findSourcesOfTargetsRelaxed(
            snaps, mostRecentSources, nodePointerId);
        println("Num Nodes found: {}", mostRecentSources.size());
        EXPECT_GT(mg->getLinkCount(), 0);
        EXPECT_GT(mg->getNodeCount(), 0);
        println("After finding sources - Number links: {} Number "
                "Nodes: {}",
                mg->getLinkCount(), mg->getNodeCount());
        mg.relaxedPrune();
        println("After pruning - Number links: {} Number Nodes: {}",
                mg->getLinkCount(), mg->getNodeCount());
        EXPECT_GT(mg->getLinkCount(), 0);
        EXPECT_GT(mg->getNodeCount(), 0);
    }
    mg.strictPrune();
    println("++ FINAL ++ Number links: {} Number Nodes: {}",
            mg->getLinkCount(), mg->getNodeCount());
}
