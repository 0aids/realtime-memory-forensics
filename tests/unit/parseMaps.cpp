#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include "test_helpers.hpp"
#include "utils.hpp"
#include "types.hpp"

using namespace rmf::utils;
using namespace rmf::types;

TEST(parseMapsTest, parseOwnProcessMaps)
{
    pid_t       pid      = getpid();
    std::string mapsPath = "/proc/" + std::to_string(pid) + "/maps";

    auto        regions = ParseMaps(mapsPath);

    ASSERT_FALSE(regions.empty());

    for (const auto& mrp : regions)
    {
        EXPECT_GT(mrp.parentRegionAddress, 0);
        EXPECT_GT(mrp.relativeRegionSize, 0);
        EXPECT_NE(mrp.perms, Perms::None);
    }
}

TEST(parseMapsTest, parseTestProcessMaps)
{
    using namespace rmf::test;

    testProcess tp;
    tp.build<staticValueComponent>();
    pid_t childPid = tp.run();

    ASSERT_GT(childPid, 0);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::string mapsPath =
        "/proc/" + std::to_string(childPid) + "/maps";
    auto regions = ParseMaps(mapsPath);

    ASSERT_FALSE(regions.empty());

    auto readableRegions = regions.FilterHasPerms("r");
    ASSERT_FALSE(readableRegions.empty());

    for (const auto& mrp : readableRegions)
    {
        EXPECT_TRUE(hasPerms(mrp.perms, Perms::Read));
    }

    tp.stop();
}

TEST(parseMapsTest, filterRegionsByPerms)
{
    using namespace rmf::test;

    testProcess tp;
    tp.build<staticValueComponent>();
    pid_t childPid = tp.run();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::string mapsPath =
        "/proc/" + std::to_string(childPid) + "/maps";
    auto regions = ParseMaps(mapsPath);

    auto readable   = regions.FilterHasPerms("r");
    auto writable   = regions.FilterHasPerms("w");
    auto executable = regions.FilterHasPerms("x");

    EXPECT_GE(readable.size(), 1);
    EXPECT_GE(writable.size(), 1);
    EXPECT_GE(executable.size(), 1);

    tp.stop();
}

TEST(parseMapsTest, filterRegionsBySize)
{
    using namespace rmf::test;

    testProcess tp;
    tp.build<staticValueComponent>();
    pid_t childPid = tp.run();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::string mapsPath =
        "/proc/" + std::to_string(childPid) + "/maps";
    auto regions = ParseMaps(mapsPath);

    auto largeRegions = regions.FilterMinSize(0x1000);
    auto smallRegions = regions.FilterMaxSize(0x1000);

    EXPECT_GE(largeRegions.size(), 1);

    for (const auto& mrp : largeRegions)
    {
        EXPECT_GE(mrp.relativeRegionSize, 0x1000);
    }

    for (const auto& mrp : smallRegions)
    {
        EXPECT_LE(mrp.relativeRegionSize, 0x1000);
    }

    tp.stop();
}
