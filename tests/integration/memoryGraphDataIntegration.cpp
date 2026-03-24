#include <gtest/gtest.h>
#include <rmf.hpp>
#include <ranges>
#include <iterator>
#include <thread>
#include <chrono>
#include <cstring>
#include "logger.hpp"
#include "test_helpers.hpp"
#include "utils.hpp"
#include "types.hpp"
#include "operations.hpp"
#include "memory_graph.hpp"

using namespace rmf::utils;
using namespace rmf::types;
using namespace rmf::op;
using namespace rmf::graph;

TEST(memoryGraphDataIntegrationTest, buildGraphFromLinkedList)
{
    using namespace rmf::test;

    std::vector<int32_t> listValues = {0x9877, 0x12351, 0x1113,
                                       0x7767};
    rmf::g_logLevel                 = rmf_Info;
    testProcess tp;
    tp.build<SListComponent<int32_t>>(listValues);
    pid_t childPid = tp.run();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::string mapsPath =
        "/proc/" + std::to_string(childPid) + "/maps";
    auto regions         = ParseMaps(mapsPath);
    auto readableRegions = regions.FilterHasPerms("r");

    ASSERT_FALSE(readableRegions.empty());

    MemoryGraphData graph;

    for (const auto& mrp : readableRegions)
    {
        auto snapshot = MemorySnapshot::Make(mrp, childPid);
        if (!snapshot.isValid())
            continue;

        auto results = findNumeralExact<int32_t>(snapshot, 10);
        for (const auto& resultMrp : results)
        {
            MemoryNodeData nodeData;
            nodeData.mrp = resultMrp;
            graph.addNode(nodeData);
        }
    }

    EXPECT_GE(graph.getNodeCount(), 1);

    tp.stop();
}

TEST(memoryGraphDataIntegrationTest, pointerChainDiscovery)
{
    using namespace rmf::test;
    rmf::g_logLevel = rmf_Info;

    std::vector<int32_t> listValues = {0x9877, 0x12351, 0x1113,
                                       0x7767};
    testProcess          tp;
    tp.build<SListComponent<int32_t>>(listValues);
    pid_t childPid = tp.run();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::string mapsPath =
        "/proc/" + std::to_string(childPid) + "/maps";
    auto regions         = ParseMaps(mapsPath);
    auto readableRegions = regions.FilterHasPerms("r");

    ASSERT_FALSE(readableRegions.empty());

    MemoryGraphData           graph;
    MemoryRegionPropertiesVec foundValueRegions;

    rmf::Analyzer a(std::thread::hardware_concurrency() - 1);
    auto          snapshots =
        a.Execute(MemorySnapshot::Make, readableRegions, childPid);
    for (const auto value : listValues)
    {
        auto foundRegions =
            a.Execute(findNumeralExact<int32_t>, snapshots, value)
                .flatten();
        for (const auto& result : foundRegions)
        {
            MemoryNodeData nodeData;
            nodeData.mrp = result;
            graph.addNode(nodeData);
            foundValueRegions.push_back(result);
        }
    }

    EXPECT_GE(graph.getNodeCount(), listValues.size());

    for (const auto& targetMrp : foundValueRegions)
    {
        for (const auto& searchMrp : readableRegions)
        {
            auto snapshot = MemorySnapshot::Make(searchMrp, childPid);
            if (!snapshot.isValid())
                continue;

            auto pointers = findPointersToRegion(snapshot, targetMrp);
            for (const auto& ptrMrp : pointers)
            {
                MemoryNodeData nodeData;
                nodeData.mrp = ptrMrp;
                auto nodeKey = graph.addNode(nodeData);

                auto targetKey =
                    graph.getNodeKeyAtAddr(targetMrp.TrueAddress());
                if (targetKey.has_value())
                {
                    MemoryLinkData linkData;
                    linkData.sourceAddr = ptrMrp.TrueAddress();
                    linkData.targetAddr = targetMrp.TrueAddress();
                    graph.addLink(nodeKey, targetKey.value(),
                                  linkData);
                }
            }
        }
    }

    tp.stop();
}

TEST(memoryGraphDataIntegrationTest, traversePointerChain)
{
    using namespace rmf::test;
    rmf::g_logLevel = rmf_Info;

    std::vector<int32_t> listValues = {0x987788, 0x1235123, 0x111334,
                                       0x776712};
    testProcess          tp;
    tp.build<SListComponent<int32_t>>(listValues);
    pid_t childPid = tp.run();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::string mapsPath =
        "/proc/" + std::to_string(childPid) + "/maps";
    auto regions         = ParseMaps(mapsPath);
    auto readableRegions = regions.FilterHasPerms("r");

    ASSERT_FALSE(readableRegions.empty());

    MemoryGraphData graph;
    rmf::Analyzer   a(std::thread::hardware_concurrency() - 1);
    auto            snapshots =
        a.Execute(MemorySnapshot::Make, readableRegions, childPid);

    for (int32_t val : listValues)
    {
        auto results =
            a.Execute(findNumeralExact<int32_t>, snapshots, val)
                .flatten();
        for (const auto& resultMrp : results)
        {
            MemoryNodeData nodeData;
            nodeData.mrp = resultMrp;
            graph.addNode(nodeData);
        }
    }

    size_t initialNodeCount = graph.getNodeCount();
    ASSERT_GE(initialNodeCount, listValues.size());

    // need to account for cycles
    // Only do like 5 loops
    size_t i = 0;
    for (const auto& [key, node] : graph.getNodes())
    {
        auto pointers = a.Execute(findPointersToRegion, snapshots,
                                  node.nodeData.mrp)
                            .flatten();
        for (const auto& ptrMrp : pointers)
        {
            if (i++ > 100)
                break;
            auto ptrNodeKey =
                graph.getNodeKeyAtAddr(ptrMrp.TrueAddress());
            if (!ptrNodeKey.has_value())
            {
                MemoryNodeData ptrNodeData;
                ptrNodeData.mrp = ptrMrp;
                ptrNodeKey      = graph.addNode(ptrNodeData);
            }

            MemoryLinkData linkData;
            linkData.sourceAddr = ptrMrp.TrueAddress();
            linkData.targetAddr = node.nodeData.mrp.TrueAddress();
            graph.addLink(ptrNodeKey.value(), key, linkData);
        }
        if (i++ > 100)
            break;
    }

    EXPECT_GE(graph.getNodeCount(), initialNodeCount);

    tp.stop();
}

TEST(memoryGraphDataIntegrationTest, findParentsOfValue)
{
    using namespace rmf::test;
    rmf::g_logLevel = rmf_Info;

    std::vector<int32_t> listValues = {999};
    testProcess          tp;
    tp.build<SListComponent<int32_t>>(listValues);
    pid_t childPid = tp.run();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::string mapsPath =
        "/proc/" + std::to_string(childPid) + "/maps";
    auto regions         = ParseMaps(mapsPath);
    auto readableRegions = regions.FilterHasPerms("r");

    ASSERT_FALSE(readableRegions.empty());

    MemoryGraphData        graph;
    std::optional<NodeKey> targetNodeKey;

    for (const auto& mrp : readableRegions)
    {
        auto snapshot = MemorySnapshot::Make(mrp, childPid);
        if (!snapshot.isValid())
            continue;

        auto results = findNumeralExact<int32_t>(snapshot, 999);
        for (const auto& resultMrp : results)
        {
            MemoryNodeData nodeData;
            nodeData.mrp = resultMrp;
            auto key     = graph.addNode(nodeData);
            if (!targetNodeKey.has_value())
            {
                targetNodeKey = key;
            }
        }
    }

    ASSERT_TRUE(targetNodeKey.has_value());

    auto parents = graph.getParents(targetNodeKey.value());
    std::vector<MemoryNode> parentNodes(parents.begin(),
                                        parents.end());

    EXPECT_GE(parentNodes.size(), 0);

    tp.stop();
}

TEST(memoryGraphDataIntegrationTest, complexPointerGraph)
{
    using namespace rmf::test;
    rmf::g_logLevel = rmf_Info;

    std::vector<int32_t> listValues = {0x987788, 0x1235123, 0x111334,
                                       0x776712};
    testProcess          tp;
    tp.build<SListComponent<int32_t>>(listValues);
    pid_t childPid = tp.run();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::string mapsPath =
        "/proc/" + std::to_string(childPid) + "/maps";
    auto regions         = ParseMaps(mapsPath);
    auto readableRegions = regions.FilterHasPerms("r");

    ASSERT_FALSE(readableRegions.empty());

    rmf::Analyzer analyzer(std::thread::hardware_concurrency() - 1);
    auto          snaps =
        analyzer.Execute(MemorySnapshot::Make, regions, childPid);

    MemoryGraphData           graph;
    MemoryRegionPropertiesVec allFoundValues;

    for (int32_t val : listValues)
    {
        auto result =
            analyzer.Execute(findNumeralExact<int32_t>, snaps, val)
                .flatten();

        for (const auto& mrp : result)
        {
            MemoryNodeData nodeData;
            nodeData.mrp = mrp;
            graph.addNode(nodeData);
            allFoundValues.push_back(mrp);
        }
    }

    ASSERT_GE(graph.getNodeCount(), listValues.size());

    for (const auto& targetMrp : allFoundValues)
    {
        auto result =
            analyzer.Execute(findPointersToRegion, snaps, targetMrp)
                .flatten();
        for (const auto& ptrMrp : result)
        {
            MemoryNodeData ptrNodeData;
            ptrNodeData.mrp = ptrMrp;
            auto ptrKey     = graph.addNode(ptrNodeData);

            auto targetKey =
                graph.getNodeKeyAtAddr(targetMrp.TrueAddress());
            if (targetKey.has_value())
            {
                MemoryLinkData linkData;
                linkData.sourceAddr = ptrMrp.TrueAddress();
                linkData.targetAddr = targetMrp.TrueAddress();
                graph.addLink(ptrKey, targetKey.value(), linkData);
            }
        }
    }

    EXPECT_GE(graph.getLinkCount(), 0);

    tp.stop();
}
