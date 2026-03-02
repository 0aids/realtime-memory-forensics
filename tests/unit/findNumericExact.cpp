#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <cstring>
#include "test_helpers.hpp"
#include "utils.hpp"
#include "types.hpp"
#include "operations.hpp"

using namespace rmf::utils;
using namespace rmf::types;
using namespace rmf::op;

TEST(findNumericExactTest, findInt32InTestProcess)
{
    using namespace rmf::test;

    testProcess tp;
    tp.build<staticValueComponent>();
    pid_t childPid = tp.run();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::string mapsPath =
        "/proc/" + std::to_string(childPid) + "/maps";
    auto regions         = ParseMaps(mapsPath, childPid);
    auto readableRegions = regions.FilterHasPerms("r");

    ASSERT_FALSE(readableRegions.empty());

    int32_t targetValue = 42;
    bool    found       = false;

    for (const auto& mrp : readableRegions)
    {
        auto snapshot = MemorySnapshot::Make(mrp);
        if (!snapshot.isValid())
            continue;

        auto results =
            findNumeralExact<int32_t>(snapshot, targetValue);
        if (!results.empty())
        {
            found = true;
            break;
        }
    }

    EXPECT_TRUE(found);
    tp.stop();
}

TEST(findNumericExactTest, findInt64InTestProcess)
{
    using namespace rmf::test;

    testProcess tp;
    tp.build<staticValueComponent>();
    pid_t childPid = tp.run();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::string mapsPath =
        "/proc/" + std::to_string(childPid) + "/maps";
    auto regions         = ParseMaps(mapsPath, childPid);
    auto readableRegions = regions.FilterHasPerms("r");

    ASSERT_FALSE(readableRegions.empty());

    int64_t targetValue = 1234567890123;
    bool    found       = false;

    for (const auto& mrp : readableRegions)
    {
        auto snapshot = MemorySnapshot::Make(mrp);
        if (!snapshot.isValid())
            continue;

        auto results =
            findNumeralExact<int64_t>(snapshot, targetValue);
        if (!results.empty())
        {
            found = true;
            break;
        }
    }

    EXPECT_TRUE(found);
    tp.stop();
}

TEST(findNumericExactTest, findNonExistentValue)
{
    using namespace rmf::test;

    testProcess tp;
    tp.build<staticValueComponent>();
    pid_t childPid = tp.run();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::string mapsPath =
        "/proc/" + std::to_string(childPid) + "/maps";
    auto regions         = ParseMaps(mapsPath, childPid);
    auto readableRegions = regions.FilterHasPerms("r");

    ASSERT_FALSE(readableRegions.empty());

    int32_t targetValue = 999999999;

    for (const auto& mrp : readableRegions)
    {
        auto snapshot = MemorySnapshot::Make(mrp);
        if (!snapshot.isValid())
            continue;

        auto results =
            findNumeralExact<int32_t>(snapshot, targetValue);
        EXPECT_TRUE(results.empty());
    }

    tp.stop();
}
