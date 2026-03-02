#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include "test_helpers.hpp"
#include "utils.hpp"
#include "types.hpp"
#include "operations.hpp"

using namespace rmf::utils;
using namespace rmf::types;
using namespace rmf::op;

TEST(findStringTest, findStringInTestProcess)
{
    using namespace rmf::test;

    testProcess tp;
    tp.build<staticStringTestComponent>();
    pid_t childPid = tp.run();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::string mapsPath =
        "/proc/" + std::to_string(childPid) + "/maps";
    auto regions         = ParseMaps(mapsPath, childPid);
    auto readableRegions = regions.FilterHasPerms("r");

    ASSERT_FALSE(readableRegions.empty());

    std::string targetString = "I am a small string";
    bool        found        = false;

    for (const auto& mrp : readableRegions)
    {
        auto snapshot = MemorySnapshot::Make(mrp);
        if (!snapshot.isValid())
            continue;

        auto results = findString(snapshot, targetString);
        if (!results.empty())
        {
            found = true;
            break;
        }
    }

    EXPECT_TRUE(found);
    tp.stop();
}

TEST(findStringTest, findLoremIpsum)
{
    using namespace rmf::test;

    testProcess tp;
    tp.build<staticStringTestComponent>();
    pid_t childPid = tp.run();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::string mapsPath =
        "/proc/" + std::to_string(childPid) + "/maps";
    auto regions         = ParseMaps(mapsPath, childPid);
    auto readableRegions = regions.FilterHasPerms("r");

    ASSERT_FALSE(readableRegions.empty());

    std::string targetString = "lorem ipsum";
    bool        found        = false;

    for (const auto& mrp : readableRegions)
    {
        auto snapshot = MemorySnapshot::Make(mrp);
        if (!snapshot.isValid())
            continue;

        auto results = findString(snapshot, targetString);
        if (!results.empty())
        {
            found = true;
            break;
        }
    }

    EXPECT_TRUE(found);
    tp.stop();
}

TEST(findStringTest, findNonExistentString)
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

    std::string targetString = "THIS_STRING_DOES_NOT_EXIST_IN_MEMORY";

    for (const auto& mrp : readableRegions)
    {
        auto snapshot = MemorySnapshot::Make(mrp);
        if (!snapshot.isValid())
            continue;

        auto results = findString(snapshot, targetString);
        EXPECT_TRUE(results.empty());
    }

    tp.stop();
}
