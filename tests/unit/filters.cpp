#include <gtest/gtest.h>
#include "types.hpp"
#include "utils.hpp"

using namespace magic_enum::bitwise_operators;

using namespace rmf::types;
using namespace rmf::utils;

TEST(filtersTest, filterMinSize)
{
    MemoryRegionPropertiesVec regions = {
        {0x1000, 0x1000, 0, 0x100,
         std::make_shared<const std::string>("small"),  Perms::Read,
         0},
        {0x2000, 0x2000, 0, 0x1000,
         std::make_shared<const std::string>("medium"), Perms::Read,
         0},
        {0x3000, 0x3000, 0, 0x10000,
         std::make_shared<const std::string>("large"),  Perms::Read,
         0},
    };

    auto result = FilterMinSize(regions, 0x1000);

    ASSERT_EQ(result.size(), 2);
    EXPECT_GE(result[0].relativeRegionSize, 0x1000);
    EXPECT_GE(result[1].relativeRegionSize, 0x1000);
}

TEST(filtersTest, filterMaxSize)
{
    MemoryRegionPropertiesVec regions = {
        {0x1000, 0x1000, 0, 0x100,
         std::make_shared<const std::string>("small"),  Perms::Read,
         0},
        {0x2000, 0x2000, 0, 0x1000,
         std::make_shared<const std::string>("medium"), Perms::Read,
         0},
        {0x3000, 0x3000, 0, 0x10000,
         std::make_shared<const std::string>("large"),  Perms::Read,
         0},
    };

    auto result = FilterMaxSize(regions, 0x1000);

    ASSERT_EQ(result.size(), 2);
    EXPECT_LE(result[0].relativeRegionSize, 0x1000);
    EXPECT_LE(result[1].relativeRegionSize, 0x1000);
}

TEST(filtersTest, filterName)
{
    MemoryRegionPropertiesVec regions = {
        {0x1000, 0x1000, 0, 0x100,
         std::make_shared<const std::string>("test1"), Perms::Read,
         0},
        {0x2000, 0x2000, 0, 0x100,
         std::make_shared<const std::string>("test2"), Perms::Read,
         0},
        {0x3000, 0x3000, 0, 0x100,
         std::make_shared<const std::string>("other"), Perms::Read,
         0},
    };

    auto result = FilterName(regions, "test1");

    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(*result[0].regionName_sp, "test1");
}

TEST(filtersTest, filterContainsName)
{
    MemoryRegionPropertiesVec regions = {
        {0x1000, 0x1000, 0, 0x100,
         std::make_shared<const std::string>("test1"), Perms::Read,
         0},
        {0x2000, 0x2000, 0, 0x100,
         std::make_shared<const std::string>("test2"), Perms::Read,
         0},
        {0x3000, 0x3000, 0, 0x100,
         std::make_shared<const std::string>("other"), Perms::Read,
         0},
    };

    auto result = FilterContainsName(regions, "test");

    ASSERT_EQ(result.size(), 2);
    EXPECT_TRUE(result[0].regionName_sp->contains("test") ||
                result[1].regionName_sp->contains("test"));
}

TEST(filtersTest, filterPermsExact)
{
    MemoryRegionPropertiesVec regions = {
        {0x1000, 0x1000, 0, 0x100,
         std::make_shared<const std::string>("r"),   Perms::Read, 0},
        {0x2000, 0x2000, 0, 0x100,
         std::make_shared<const std::string>("rw"),
         Perms::Read | Perms::Write,                              0},
        {0x3000, 0x3000, 0, 0x100,
         std::make_shared<const std::string>("r--"), Perms::Read, 0},
    };

    auto result = FilterPerms(regions, "r--");

    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0].perms, Perms::Read);
}

TEST(filtersTest, filterHasPerms)
{
    MemoryRegionPropertiesVec regions = {
        {0x1000, 0x1000, 0, 0x100,
         std::make_shared<const std::string>("r"),   Perms::Read, 0},
        {0x2000, 0x2000, 0, 0x100,
         std::make_shared<const std::string>("rw"),
         Perms::Read | Perms::Write,                              0},
        {0x3000, 0x3000, 0, 0x100,
         std::make_shared<const std::string>("rwx"),
         Perms::Read | Perms::Write | Perms::Execute,             0},
    };

    auto result = FilterHasPerms(regions, "r");

    ASSERT_EQ(result.size(), 3);
}

TEST(filtersTest, filterNotPerms)
{
    MemoryRegionPropertiesVec regions = {
        {0x1000, 0x1000, 0, 0x100,
         std::make_shared<const std::string>("r"),   Perms::Read, 0},
        {0x2000, 0x2000, 0, 0x100,
         std::make_shared<const std::string>("rw"),
         Perms::Read | Perms::Write,                              0},
        {0x3000, 0x3000, 0, 0x100,
         std::make_shared<const std::string>("rwx"),
         Perms::Read | Perms::Write | Perms::Execute,             0},
    };

    auto result = FilterNotPerms(regions, "x");

    ASSERT_EQ(result.size(), 2);
    for (const auto& mrp : result)
    {
        EXPECT_FALSE(hasPerms(mrp.perms, Perms::Execute));
    }
}

TEST(filtersTest, getRegionContainingAddress)
{
    MemoryRegionPropertiesVec regions = {
        {0x1000, 0x1000, 0, 0x100,
         std::make_shared<const std::string>("region1"), Perms::Read,
         0},
        {0x2000, 0x2000, 0, 0x100,
         std::make_shared<const std::string>("region2"), Perms::Read,
         0},
        {0x3000, 0x3000, 0, 0x100,
         std::make_shared<const std::string>("region3"), Perms::Read,
         0},
    };

    auto result = regions.GetRegionContainingAddress(0x1050);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result->regionName_sp, "region1");

    result = regions.GetRegionContainingAddress(0x2050);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result->regionName_sp, "region2");

    result = regions.GetRegionContainingAddress(0x4000);
    ASSERT_FALSE(result.has_value());
}

TEST(filtersTest, parsePerms)
{
    EXPECT_EQ(ParsePerms("r--"), Perms::Read);
    EXPECT_EQ(ParsePerms("rw-"), Perms::Read | Perms::Write);
    EXPECT_EQ(ParsePerms("rwx"),
              Perms::Read | Perms::Write | Perms::Execute);
    EXPECT_EQ(ParsePerms("r-s"), Perms::Read | Perms::Shared);
}
