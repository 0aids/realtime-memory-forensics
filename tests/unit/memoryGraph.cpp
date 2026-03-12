#include <gtest/gtest.h>
#include "memory_graph.hpp"

using namespace rmf::graph;

TEST(memoryGraphTest, addRegion)
{
    MemoryGraph      graph;

    MemoryRegionData data;
    data.mrp = {
        0x1000,
        0x1000,
        0,
        0x1000,
        std::make_shared<const std::string>("test"),
        rmf::types::Perms::Read,
    };
    data.name = "testRegion";

    auto id = graph.RegionAdd(data);

    EXPECT_GT(id, 0);

    auto region = graph.RegionGetFromID(id);
    ASSERT_TRUE(region.has_value());
    EXPECT_EQ((*region)->data.name, "testRegion");
}

TEST(memoryGraphTest, getRegionAtAddress)
{
    MemoryGraph      graph;

    MemoryRegionData data1;
    data1.mrp = {
        0x1000,
        0x1000,
        0,
        0x1000,
        std::make_shared<const std::string>("region1"),
        rmf::types::Perms::Read,
    };
    data1.name = "region1";
    graph.RegionAdd(data1);

    MemoryRegionData data2;
    data2.mrp = {
        0x2000,
        0x1000,
        0,
        0x1000,
        std::make_shared<const std::string>("region2"),
        rmf::types::Perms::Read,
    };
    data2.name = "region2";
    graph.RegionAdd(data2);

    auto region = graph.RegionGetRegionAtAddress(0x1500);
    ASSERT_FALSE(region.has_value());

    region = graph.RegionGetRegionAtAddress(0x2500);
    ASSERT_FALSE(region.has_value());

    region = graph.RegionGetRegionAtAddress(0x1000);
    ASSERT_TRUE(region.has_value());
    EXPECT_EQ(region.value()->data.name, "region1");

    region = graph.RegionGetRegionAtAddress(0x2000);
    ASSERT_TRUE(region.has_value());
    EXPECT_EQ(region.value()->data.name, "region2");
}

TEST(memoryGraphTest, getRegionContainingAddress)
{
    MemoryGraph      graph;

    MemoryRegionData data1;
    data1.mrp = {
        0x1000,
        0x1000,
        0,
        0x1000,
        std::make_shared<const std::string>("region1"),
        rmf::types::Perms::Read,
    };
    data1.name = "region1";
    graph.RegionAdd(data1);

    MemoryRegionData data2;
    data2.mrp = {
        0x2000,
        0x1000,
        0,
        0x1000,
        std::make_shared<const std::string>("region2"),
        rmf::types::Perms::Read,
    };
    data2.name = "region2";
    graph.RegionAdd(data2);

    auto region = graph.RegionGetRegionContainingAddress(0x3000);
    ASSERT_FALSE(region.has_value());

    region = graph.RegionGetRegionContainingAddress(0x0fff);
    ASSERT_FALSE(region.has_value());

    region = graph.RegionGetRegionContainingAddress(0x2fff);
    ASSERT_TRUE(region.has_value());
    EXPECT_EQ(region.value()->data.name, "region2");

    region = graph.RegionGetRegionContainingAddress(0x1500);
    ASSERT_TRUE(region.has_value());
    EXPECT_EQ(region.value()->data.name, "region1");
}

TEST(memoryGraphTest, addLink)
{
    MemoryGraph      graph;

    MemoryRegionData data1;
    data1.mrp = {
        0x1000,
        0x1000,
        0,
        0x1000,
        std::make_shared<const std::string>("source"),
        rmf::types::Perms::Read,
    };
    data1.name           = "source";
    auto             id1 = graph.RegionAdd(data1);

    MemoryRegionData data2;
    data2.mrp = {
        0x2000,
        0x1000,
        0,
        0x1000,
        std::make_shared<const std::string>("target"),
        rmf::types::Perms::Read,
    };
    data2.name         = "target";
    auto           id2 = graph.RegionAdd(data2);

    MemoryLinkData linkData;
    linkData.sourceID   = id1;
    linkData.targetID   = id2;
    linkData.sourceAddr = 0x1000;
    linkData.targetAddr = 0x2000;
    linkData.name       = "testLink";

    auto linkId = graph.LinkNaiveAdd(linkData);

    EXPECT_GT(linkId, 0);

    auto link = graph.LinkGetFromID(linkId);
    ASSERT_TRUE(link.has_value());
    EXPECT_EQ((*link)->data.name, "testLink");
}

TEST(memoryGraphTest, deleteRegion)
{
    MemoryGraph      graph;

    MemoryRegionData data;
    data.mrp = {
        0x1000,
        0x1000,
        0,
        0x1000,
        std::make_shared<const std::string>("test"),
        rmf::types::Perms::Read,
    };
    data.name = "testRegion";

    auto id = graph.RegionAdd(data);
    EXPECT_GT(id, 0);

    graph.RegionDelete(id);

    auto region = graph.RegionGetFromID(id);
    ASSERT_FALSE(region.has_value());
}

TEST(memoryGraphTest, deleteLink)
{
    MemoryGraph      graph;

    MemoryRegionData data1;
    data1.mrp = {
        0x1000,
        0x1000,
        0,
        0x1000,
        std::make_shared<const std::string>("source"),
        rmf::types::Perms::Read,
    };
    data1.name           = "source";
    auto             id1 = graph.RegionAdd(data1);

    MemoryRegionData data2;
    data2.mrp          = {0x2000,
                          0x1000,
                          0,
                          0x1000,
                          std::make_shared<const std::string>("target"),
                          rmf::types::Perms::Read};
    data2.name         = "target";
    auto           id2 = graph.RegionAdd(data2);

    MemoryLinkData linkData;
    linkData.sourceID   = id1;
    linkData.targetID   = id2;
    linkData.sourceAddr = 0x1000;
    linkData.targetAddr = 0x2000;

    auto linkId = graph.LinkNaiveAdd(linkData);

    graph.LinkDelete(linkId);

    auto link = graph.LinkGetFromID(linkId);
    ASSERT_FALSE(link.has_value());
}

TEST(memoryGraphTest, getAllRegions)
{
    MemoryGraph graph;

    for (uintptr_t i = 0; i < 5; i++)
    {
        MemoryRegionData data;
        data.mrp  = {0x1000 * (i + 1),
                     0x1000,
                     0,
                     0x1000,
                     std::make_shared<const std::string>(
                        "region" + std::to_string(i)),
                     rmf::types::Perms::Read};
        data.name = "region" + std::to_string(i);
        graph.RegionAdd(data);
    }

    auto views = graph.RegionsGetViews();
    int  count = 0;
    for (auto& region : views)
    {
        (void)region;
        count++;
    }

    EXPECT_EQ(count, 5);
}
