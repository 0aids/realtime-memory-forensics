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

TEST(memoryGraphDataTest, stateQueriesEmpty)
{
    MemoryGraphData graph;

    EXPECT_TRUE(graph.isEmpty());
    EXPECT_EQ(graph.getNodeCount(), 0);
    EXPECT_EQ(graph.getLinkCount(), 0);
}

TEST(memoryGraphDataTest, stateQueriesAfterAdd)
{
    MemoryGraphData graph;

    MemoryNodeData  data1;
    data1.mrp = {
        0x1000,
        0x1000,
        0,
        0x1000,
        std::make_shared<const std::string>("node1"),
        rmf::types::Perms::Read,
    };
    auto id1 = graph.addNode(data1);

    EXPECT_FALSE(graph.isEmpty());
    EXPECT_EQ(graph.getNodeCount(), 1);
    EXPECT_EQ(graph.getLinkCount(), 0);
    EXPECT_TRUE(graph.isValidNode(id1));
}

TEST(memoryGraphDataTest, stateQueriesWithLinks)
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

    EXPECT_FALSE(graph.isEmpty());
    EXPECT_EQ(graph.getNodeCount(), 2);
    EXPECT_EQ(graph.getLinkCount(), 1);
    EXPECT_TRUE(graph.isValidNode(id1));
    EXPECT_TRUE(graph.isValidNode(id2));
    EXPECT_TRUE(graph.isValidLink(linkId.value()));
}

TEST(memoryGraphDataTest, stateQueriesAfterRemove)
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

    EXPECT_TRUE(graph.isEmpty());
    EXPECT_EQ(graph.getNodeCount(), 0);
    EXPECT_FALSE(graph.isValidNode(id));
}

TEST(memoryGraphDataTest, stateQueriesInvalidKeys)
{
    MemoryGraphData     graph;

    rmf::utils::SlotKey invalidNodeKey{9999, 9999};
    rmf::utils::SlotKey invalidLinkKey{9999, 9999};

    EXPECT_FALSE(graph.isValidNode(invalidNodeKey));
    EXPECT_FALSE(graph.isValidLink(invalidLinkKey));
    EXPECT_FALSE(graph.getNode(invalidNodeKey).has_value());
    EXPECT_FALSE(graph.getLink(invalidLinkKey).has_value());
}

TEST(memoryGraphDataTest, updateNodeData)
{
    MemoryGraphData graph;

    MemoryNodeData  data1;
    data1.mrp = {
        0x1000,
        0x1000,
        0,
        0x1000,
        std::make_shared<const std::string>("original"),
        rmf::types::Perms::Read,
    };
    auto id1 = graph.addNode(data1);

    auto node = graph.getNode(id1);
    ASSERT_TRUE(node.has_value());
    EXPECT_EQ(*node.value().nodeData.mrp.regionName_sp, "original");

    MemoryNodeData data2;
    data2.mrp = {
        0x2000,
        0x2000,
        0,
        0x2000,
        std::make_shared<const std::string>("updated"),
        rmf::types::Perms::Write,
    };
    auto newKey = graph.updateNodeData(id1, data2);
    ASSERT_TRUE(newKey.has_value());

    node = graph.getNode(newKey.value());
    ASSERT_TRUE(node.has_value());
    EXPECT_EQ(*node.value().nodeData.mrp.regionName_sp, "updated");
    EXPECT_EQ(node.value().nodeData.mrp.TrueAddress(), 0x2000);
}

TEST(memoryGraphDataTest, updateNodeDataInvalidKey)
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

    rmf::utils::SlotKey invalidKey{9999, 9999};
    auto result = graph.updateNodeData(invalidKey, data);
    EXPECT_FALSE(result.has_value());
}

TEST(memoryGraphDataTest, updateLinkData)
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
    ASSERT_TRUE(linkId.has_value());

    auto link = graph.getLink(linkId.value());
    ASSERT_TRUE(link.has_value());

    MemoryLinkData newLinkData;
    newLinkData.sourceAddr = 0x1000;
    newLinkData.targetAddr = 0x2000;

    auto newLinkId =
        graph.updateLinkData(linkId.value(), newLinkData);
    ASSERT_TRUE(newLinkId.has_value());

    link = graph.getLink(newLinkId.value());
    ASSERT_TRUE(link.has_value());
}

TEST(memoryGraphDataTest, updateLinkDataInvalidKey)
{
    MemoryGraphData graph;

    MemoryLinkData  linkData;
    linkData.sourceAddr = 0x1000;
    linkData.targetAddr = 0x2000;

    rmf::utils::SlotKey invalidKey{9999, 9999};
    auto result = graph.updateLinkData(invalidKey, linkData);
    EXPECT_FALSE(result.has_value());
}

TEST(memoryGraphDataTest, getOutgoingLinks)
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
        std::make_shared<const std::string>("target1"),
        rmf::types::Perms::Read,
    };
    auto           id2 = graph.addNode(data2);

    MemoryNodeData data3;
    data3.mrp = {
        0x3000,
        0x1000,
        0,
        0x1000,
        std::make_shared<const std::string>("target2"),
        rmf::types::Perms::Read,
    };
    auto           id3 = graph.addNode(data3);

    MemoryLinkData linkData1;
    linkData1.sourceAddr = 0x1000;
    linkData1.targetAddr = 0x2000;
    graph.addLink(id1, id2, linkData1);

    MemoryLinkData linkData2;
    linkData2.sourceAddr = 0x1000;
    linkData2.targetAddr = 0x3000;
    graph.addLink(id1, id3, linkData2);

    auto outgoing = graph.getOutgoingLinks(id1);
    std::vector<rmf::utils::SlotKey> outgoingKeys(outgoing.begin(),
                                                  outgoing.end());
    EXPECT_EQ(outgoingKeys.size(), 2);
}

TEST(memoryGraphDataTest, getIncomingLinks)
{
    MemoryGraphData graph;

    MemoryNodeData  data1;
    data1.mrp = {
        0x1000,
        0x1000,
        0,
        0x1000,
        std::make_shared<const std::string>("source1"),
        rmf::types::Perms::Read,
    };
    auto           id1 = graph.addNode(data1);

    MemoryNodeData data2;
    data2.mrp = {
        0x2000,
        0x1000,
        0,
        0x1000,
        std::make_shared<const std::string>("source2"),
        rmf::types::Perms::Read,
    };
    auto           id2 = graph.addNode(data2);

    MemoryNodeData data3;
    data3.mrp = {
        0x3000,
        0x1000,
        0,
        0x1000,
        std::make_shared<const std::string>("target"),
        rmf::types::Perms::Read,
    };
    auto           id3 = graph.addNode(data3);

    MemoryLinkData linkData1;
    linkData1.sourceAddr = 0x1000;
    linkData1.targetAddr = 0x3000;
    graph.addLink(id1, id3, linkData1);

    MemoryLinkData linkData2;
    linkData2.sourceAddr = 0x2000;
    linkData2.targetAddr = 0x3000;
    graph.addLink(id2, id3, linkData2);

    auto incoming = graph.getIncomingLinks(id3);
    std::vector<rmf::utils::SlotKey> incomingKeys(incoming.begin(),
                                                  incoming.end());
    EXPECT_EQ(incomingKeys.size(), 2);
}

TEST(memoryGraphDataTest, getChildren)
{
    MemoryGraphData graph;

    MemoryNodeData  dataParent;
    dataParent.mrp = {
        0x1000,
        0x1000,
        0,
        0x1000,
        std::make_shared<const std::string>("parent"),
        rmf::types::Perms::Read,
    };
    auto           parentId = graph.addNode(dataParent);

    MemoryNodeData dataChild1;
    dataChild1.mrp = {
        0x2000,
        0x1000,
        0,
        0x1000,
        std::make_shared<const std::string>("child1"),
        rmf::types::Perms::Read,
    };
    auto           child1Id = graph.addNode(dataChild1);

    MemoryNodeData dataChild2;
    dataChild2.mrp = {
        0x3000,
        0x1000,
        0,
        0x1000,
        std::make_shared<const std::string>("child2"),
        rmf::types::Perms::Read,
    };
    auto           child2Id = graph.addNode(dataChild2);

    MemoryLinkData linkData;
    linkData.sourceAddr = 0x1000;
    linkData.targetAddr = 0x2000;
    graph.addLink(parentId, child1Id, linkData);

    linkData.targetAddr = 0x3000;
    graph.addLink(parentId, child2Id, linkData);

    auto                    children = graph.getChildren(parentId);
    std::vector<MemoryNode> childNodes(children.begin(),
                                       children.end());
    EXPECT_EQ(childNodes.size(), 2);
}

TEST(memoryGraphDataTest, getParents)
{
    MemoryGraphData graph;

    MemoryNodeData  data1;
    data1.mrp = {
        0x1000,
        0x1000,
        0,
        0x1000,
        std::make_shared<const std::string>("parent1"),
        rmf::types::Perms::Read,
    };
    auto           parent1Id = graph.addNode(data1);

    MemoryNodeData data2;
    data2.mrp = {
        0x2000,
        0x1000,
        0,
        0x1000,
        std::make_shared<const std::string>("parent2"),
        rmf::types::Perms::Read,
    };
    auto           parent2Id = graph.addNode(data2);

    MemoryNodeData dataChild;
    dataChild.mrp = {
        0x3000,
        0x1000,
        0,
        0x1000,
        std::make_shared<const std::string>("child"),
        rmf::types::Perms::Read,
    };
    auto           childId = graph.addNode(dataChild);

    MemoryLinkData linkData;
    linkData.sourceAddr = 0x1000;
    linkData.targetAddr = 0x3000;
    graph.addLink(parent1Id, childId, linkData);

    linkData.sourceAddr = 0x2000;
    graph.addLink(parent2Id, childId, linkData);

    auto                    parents = graph.getParents(childId);
    std::vector<MemoryNode> parentNodes(parents.begin(),
                                        parents.end());
    EXPECT_EQ(parentNodes.size(), 2);
}

TEST(memoryGraphDataTest, traversalAfterNodeRemoval)
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
    auto linkId         = graph.addLink(id1, id2, linkData);

    auto outgoing = graph.getOutgoingLinks(id1);
    std::vector<rmf::utils::SlotKey> outgoingKeys(outgoing.begin(),
                                                  outgoing.end());
    EXPECT_EQ(outgoingKeys.size(), 1);

    graph.removeLink(linkId.value());

    outgoing     = graph.getOutgoingLinks(id1);
    outgoingKeys = std::vector<rmf::utils::SlotKey>(outgoing.begin(),
                                                    outgoing.end());
    EXPECT_EQ(outgoingKeys.size(), 0);
}

TEST(memoryGraphDataTest, addLinkInvalidNodeSource)
{
    MemoryGraphData graph;

    MemoryNodeData  data;
    data.mrp = {
        0x1000,
        0x1000,
        0,
        0x1000,
        std::make_shared<const std::string>("valid"),
        rmf::types::Perms::Read,
    };
    auto                validId = graph.addNode(data);

    rmf::utils::SlotKey invalidId{9999, 9999};

    MemoryLinkData      linkData;
    linkData.sourceAddr = 0x1000;
    linkData.targetAddr = 0x2000;

    auto result = graph.addLink(invalidId, validId, linkData);
    EXPECT_FALSE(result.has_value());
}

TEST(memoryGraphDataTest, addLinkInvalidNodeTarget)
{
    MemoryGraphData graph;

    MemoryNodeData  data;
    data.mrp = {
        0x1000,
        0x1000,
        0,
        0x1000,
        std::make_shared<const std::string>("valid"),
        rmf::types::Perms::Read,
    };
    auto                validId = graph.addNode(data);

    rmf::utils::SlotKey invalidId{9999, 9999};

    MemoryLinkData      linkData;
    linkData.sourceAddr = 0x1000;
    linkData.targetAddr = 0x2000;

    auto result = graph.addLink(validId, invalidId, linkData);
    EXPECT_FALSE(result.has_value());
}

TEST(memoryGraphDataTest, removeNodeInvalidKey)
{
    MemoryGraphData     graph;

    rmf::utils::SlotKey invalidKey{9999, 9999};
    bool                result = graph.removeNode(invalidKey);
    EXPECT_FALSE(result);
}

TEST(memoryGraphDataTest, removeLinkInvalidKey)
{
    MemoryGraphData     graph;

    rmf::utils::SlotKey invalidKey{9999, 9999};
    bool                result = graph.removeLink(invalidKey);
    EXPECT_FALSE(result);
}

TEST(memoryGraphDataTest, removeNodeTwice)
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

    bool firstRemove = graph.removeNode(id);
    EXPECT_TRUE(firstRemove);

    bool secondRemove = graph.removeNode(id);
    EXPECT_FALSE(secondRemove);
}

TEST(memoryGraphDataTest, removeLinkTwice)
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
    ASSERT_TRUE(linkId.has_value());

    bool firstRemove = graph.removeLink(linkId.value());
    EXPECT_TRUE(firstRemove);

    bool secondRemove = graph.removeLink(linkId.value());
    EXPECT_FALSE(secondRemove);
}

TEST(memoryGraphDataTest, clearGraph)
{
    MemoryGraphData graph;

    for (int i = 0; i < 5; ++i)
    {
        MemoryNodeData data;
        data.mrp = {
            uintptr_t(0x1000) + uintptr_t(i) * 0x1000,
            0x1000,
            0,
            0x1000,
            std::make_shared<const std::string>("node" +
                                                std::to_string(i)),
            rmf::types::Perms::Read,
        };
        graph.addNode(data);
    }

    EXPECT_EQ(graph.getNodeCount(), 5);
    EXPECT_FALSE(graph.isEmpty());

    graph.clear();

    EXPECT_EQ(graph.getNodeCount(), 0);
    EXPECT_EQ(graph.getLinkCount(), 0);
    EXPECT_TRUE(graph.isEmpty());
}

TEST(memoryGraphDataTest, rebuildAfterClear)
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
    auto id1 = graph.addNode(data);

    graph.clear();

    auto id2 = graph.addNode(data);
    EXPECT_TRUE(graph.isValidNode(id2));
    EXPECT_EQ(graph.getNodeCount(), 1);
}

TEST(memoryGraphDataTest, sequentialAddsAndRemoves)
{
    MemoryGraphData                  graph;

    std::vector<rmf::utils::SlotKey> keys;

    for (int i = 0; i < 10; ++i)
    {
        MemoryNodeData data;
        data.mrp = {
            uintptr_t(0x1000) + uintptr_t(i) * 0x100,
            0x100,
            0,
            0x100,
            std::make_shared<const std::string>("node" +
                                                std::to_string(i)),
            rmf::types::Perms::Read,
        };
        keys.push_back(graph.addNode(data));
    }

    EXPECT_EQ(graph.getNodeCount(), 10);

    for (size_t i = 0; i < keys.size(); i += 2)
    {
        graph.removeNode(keys[i]);
    }

    EXPECT_EQ(graph.getNodeCount(), 5);

    for (int i = 0; i < 5; ++i)
    {
        MemoryNodeData data;
        data.mrp = {
            uintptr_t(0x5000) + uintptr_t(i) * 0x100,
            0x100,
            0,
            0x100,
            std::make_shared<const std::string>("new" +
                                                std::to_string(i)),
            rmf::types::Perms::Read,
        };
        graph.addNode(data);
    }

    EXPECT_EQ(graph.getNodeCount(), 10);
}
