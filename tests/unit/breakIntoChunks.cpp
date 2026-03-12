#include <gtest/gtest.h>
#include "types.hpp"
#include "utils.hpp"

using namespace rmf::types;
using namespace rmf::utils;

TEST(breakIntoChunksTest, singleRegionIntoChunks)
{
    MemoryRegionProperties region = {
        0x1000,
        0x10000,
        0,
        0x10000,
        std::make_shared<const std::string>("test"),
        Perms::Read,
    };

    auto chunks = BreakIntoChunks(region, 0x1000);

    EXPECT_EQ(chunks.size(), 16);

    for (size_t i = 0; i < chunks.size(); i++)
    {
        EXPECT_EQ(chunks[i].relativeRegionAddress, i * 0x1000);
        EXPECT_EQ(chunks[i].relativeRegionSize, 0x1000);
    }
}

TEST(breakIntoChunksTest, regionSmallerThanChunk)
{
    MemoryRegionProperties region = {
        0x1000,
        0x500,
        0,
        0x500,
        std::make_shared<const std::string>("test"),
        Perms::Read,
    };

    auto chunks = BreakIntoChunks(region, 0x1000);

    ASSERT_EQ(chunks.size(), 1);
    EXPECT_EQ(chunks[0].relativeRegionSize, 0x500);
}

TEST(breakIntoChunksTest, withOverlap)
{
    MemoryRegionProperties region = {
        0x1000,
        0x4000,
        0,
        0x4000,
        std::make_shared<const std::string>("test"),
        Perms::Read,
    };

    auto chunks = BreakIntoChunks(region, 0x1000, 0x100);

    EXPECT_EQ(chunks.size(), 5);

    EXPECT_EQ(chunks[0].relativeRegionAddress, 0);
    EXPECT_EQ(chunks[0].relativeRegionSize, 0x1000);

    EXPECT_EQ(chunks[1].relativeRegionAddress, 0x1000 - 0x100);
    EXPECT_EQ(chunks[1].relativeRegionSize, 0x1000);
}

TEST(breakIntoChunksTest, vectorOfRegions)
{
    MemoryRegionPropertiesVec regions = {
        {
         0x1000, 0x1000,
         0,      0x1000,
         std::make_shared<const std::string>("a"),
         Perms::Read,
         },
        {
         0x2000,      0x2000,
         0, 0x2000,
         std::make_shared<const std::string>("b"),
         Perms::Read,
         },
    };

    auto chunks = BreakIntoChunks(regions, 0x800);

    ASSERT_EQ(chunks.size(), 6);
}

TEST(compressNestedMrpVecTest, flattenNestedVectors)
{
    std::vector<MemoryRegionPropertiesVec> nested = {
        MemoryRegionPropertiesVec({
                                   {
                0x1000,
                0x100,
                0,
                0x100,
                std::make_shared<const std::string>("a"),
                Perms::Read,
            }, {
                0x2000,
                0x100,
                0,
                0x100,
                std::make_shared<const std::string>("b"),
                Perms::Read,
            }, }
        ),
        MemoryRegionPropertiesVec({
                                   {
                0x3000,
                0x100,
                0,
                0x100,
                std::make_shared<const std::string>("c"),
                Perms::Read,
            }, }
        )
    };

    auto flat = CompressNestedMrpVec(nested);

    ASSERT_EQ(flat.size(), 3);
}
