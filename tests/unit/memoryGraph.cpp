#include <gtest/gtest.h>
#include <stdexcept>
#include "memory_graph.hpp"

using namespace rmf::graph;

TEST(memoryGraphDataTest, addRegion)
{
    MemoryGraphData graph;

    MemoryNodeData  data;
    data.mrp = {
        0x1000,
        0x1000,
        0,
        0x1000,
        std::make_shared<const std::string>("test"),
        rmf::types::Perms::Read,
    };

    auto key = graph.addNode(data);

    auto region = graph.getNode(key);
    ASSERT_TRUE(region.has_value());
    EXPECT_EQ(region.value().nodeData.mrp, data.mrp);
}

TEST(memoryGraphDataTest, iteratorTest)
{
    MemoryGraphData graph;

    MemoryNodeData  data;
    MemoryNodeData  data1;
    data.mrp = {
        0x1000,
        0x1000,
        0,
        0x1000,
        std::make_shared<const std::string>("test"),
        rmf::types::Perms::Read,
    };
    data1.mrp = {
        0x2000,
        0x2000,
        0,
        0x2000,
        std::make_shared<const std::string>("test2"),
        rmf::types::Perms::Read,
    };

    auto key0 = graph.addNode(data);
    auto key1 = graph.addNode(data1);
    for (const auto& [key, node] : graph.getNodes())
    {
        bool found = false;
        for (const auto& dat : {data, data1})
        {
            if (dat.mrp == node.nodeData.mrp)
            {
                found = true;
                break;
            }
        }
        ASSERT_TRUE(found);
    }
}

TEST(memoryGraphDataTest, getRegionAtAddress)
{
    MemoryGraphData graph;

    MemoryNodeData  data1;
    data1.mrp = {
        0x1000,
        0x1000,
        0,
        0x1000,
        std::make_shared<const std::string>("region1"),
        rmf::types::Perms::Read,
    };
    graph.addNode(data1);

    MemoryNodeData data2;
    data2.mrp = {
        0x2000,
        0x1000,
        0,
        0x1000,
        std::make_shared<const std::string>("region2"),
        rmf::types::Perms::Read,
    };
    graph.addNode(data2);

    auto region = graph.getNodeAtAddr(0x1500);
    ASSERT_FALSE(region.has_value());

    region = graph.getNodeAtAddr(0x2500);
    ASSERT_FALSE(region.has_value());

    region = graph.getNodeAtAddr(0x1000);
    ASSERT_TRUE(region.has_value());

    region = graph.getNodeAtAddr(0x2000);
    ASSERT_TRUE(region.has_value());
}

TEST(memoryGraphDataTest, getRegionContainingAddress)
{
    MemoryGraphData graph;

    MemoryNodeData  data1;
    data1.mrp = {
        0x1000,
        0x1000,
        0,
        0x1000,
        std::make_shared<const std::string>("region1"),
        rmf::types::Perms::Read,
    };
    graph.addNode(data1);

    MemoryNodeData data2;
    data2.mrp = {
        0x2000,
        0x1000,
        0,
        0x1000,
        std::make_shared<const std::string>("region2"),
        rmf::types::Perms::Read,
    };
    graph.addNode(data2);

    auto region = graph.getNodeContainingAddr(0x3000);
    ASSERT_FALSE(region.has_value());

    region = graph.getNodeContainingAddr(0x0fff);
    ASSERT_FALSE(region.has_value());

    region = graph.getNodeContainingAddr(0x2fff);
    ASSERT_TRUE(region.has_value());

    region = graph.getNodeContainingAddr(0x1500);
    ASSERT_TRUE(region.has_value());
}

TEST(memoryGraphDataTest, addLink)
{
    MemoryGraphData graph;

    MemoryNodeData  data1;
    data1.mrp = {
        0x1000,
        0x1000,
        0,
        0x1000,
        std::make_shared<const std::string>("source"),
        rmf::types::Perms::Read,
    };
    auto           id1 = graph.addNode(data1);

    MemoryNodeData data2;
    data2.mrp = {
        0x2000,
        0x1000,
        0,
        0x1000,
        std::make_shared<const std::string>("target"),
        rmf::types::Perms::Read,
    };
    auto           id2 = graph.addNode(data2);

    MemoryLinkData linkData;
    linkData.sourceAddr = 0x1000;
    linkData.targetAddr = 0x2000;

    auto linkId = graph.addLink(id1, id2, linkData);
    EXPECT_TRUE(linkId.has_value());

    auto link = graph.getLink(linkId.value());
    ASSERT_TRUE(link.has_value());
}

TEST(memoryGraphDataTest, removeRegion)
{
    MemoryGraphData graph;

    MemoryNodeData  data;
    data.mrp = {
        0x1000,
        0x1000,
        0,
        0x1000,
        std::make_shared<const std::string>("test"),
        rmf::types::Perms::Read,
    };

    auto id = graph.addNode(data);

    graph.removeNode(id);

    auto region = graph.getNode(id);
    ASSERT_FALSE(region.has_value());
}

TEST(memoryGraphDataTest, removeLink)
{
    MemoryGraphData graph;

    MemoryNodeData  data1;
    data1.mrp = {
        0x1000,
        0x1000,
        0,
        0x1000,
        std::make_shared<const std::string>("source"),
        rmf::types::Perms::Read,
    };
    auto           id1 = graph.addNode(data1);

    MemoryNodeData data2;
    data2.mrp          = {0x2000,
                          0x1000,
                          0,
                          0x1000,
                          std::make_shared<const std::string>("target"),
                          rmf::types::Perms::Read};
    auto           id2 = graph.addNode(data2);

    MemoryLinkData linkData;
    linkData.sourceAddr = 0x1000;
    linkData.targetAddr = 0x2000;

    auto linkId = graph.addLink(id1, id2, linkData);

    ASSERT_TRUE(linkId.has_value());

    graph.removeLink(linkId.value());

    auto link = graph.getLink(linkId.value());
    ASSERT_FALSE(link.has_value());
}
