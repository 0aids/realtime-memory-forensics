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

    // There should be 2 of this string. One from the search and the other from
    // component.
    std::string targetString = "I am a small string";
    uint8_t     numFound     = 0;

    for (const auto& mrp : readableRegions)
    {
        auto snapshot = MemorySnapshot::Make(mrp);
        if (!snapshot.isValid())
            continue;

        auto results = findString(snapshot, targetString);
        if (!results.empty())
        {
            numFound += results.size();
            continue;
        }
    }

    EXPECT_EQ(numFound, 2);
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

    // There should be 2 of this string. One from the search and the other from
    // component.
    std::string targetString =
        "lorem ipsumlorem ipsumlorem ipsumlorem ipsumlorem "
        "ipsumlorem ipsumlorem ipsumlorem ipsumlorem ipsumlorem "
        "ipsumlorem ipsumlorem ipsumlorem ipsumlorem ipsumlorem "
        "ipsumlorem ipsumlorem ipsumlorem ipsumlorem ipsumlorem "
        "ipsumlorem ipsumlorem ipsumlorem ipsumlorem ipsumlorem "
        "ipsumlorem ipsumlorem ipsumlorem ipsumlorem ipsumlorem "
        "ipsumlorem ipsumlorem ipsumlorem ipsumlorem ipsumlorem "
        "ipsumlorem ipsumlorem ipsumlorem ipsumlorem ipsumlorem "
        "ipsumlorem ipsumlorem ipsumlorem ipsumlorem ipsumlorem "
        "ipsumlorem ipsumlorem ipsumlorem ipsumlorem ipsumlorem "
        "ipsumlorem ipsumlorem ipsumlorem ipsumlorem ipsumlorem "
        "ipsumlorem ipsumlorem ipsumlorem ipsumlorem ipsumlorem "
        "ipsumlorem ipsumlorem ipsumlorem ipsumlorem ipsumlorem "
        "ipsumlorem ipsumlorem ipsumlorem ipsumlorem ipsumlorem "
        "ipsumlorem ipsumlorem ipsumlorem ipsumlorem ipsumlorem "
        "ipsumlorem ipsumlorem ipsumlorem ipsumlorem ipsumlorem "
        "ipsumlorem ipsumlorem ipsumlorem ipsumlorem ipsumlorem "
        "ipsumlorem ipsumlorem ipsumlorem ipsumlorem ipsumlorem "
        "ipsumlorem ipsumlorem ipsumlorem ipsumlorem ipsumlorem "
        "ipsumlorem ipsumlorem ipsumlorem ipsumlorem ipsumlorem "
        "ipsum";
    uint8_t numFound = 0;

    for (const auto& mrp : readableRegions)
    {
        auto snapshot = MemorySnapshot::Make(mrp);
        if (!snapshot.isValid())
            continue;

        auto results = findString(snapshot, targetString);
        if (!results.empty())
        {
            numFound += results.size();
            continue;
        }
    }

    EXPECT_EQ(numFound, 2);
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

    // this string does exist, but only once and it's right here.
    std::string targetString = "THIS_STRING_DOES_NOT_EXIST_IN_MEMORY";

    for (const auto& mrp : readableRegions)
    {
        auto snapshot = MemorySnapshot::Make(mrp);
        if (!snapshot.isValid())
            continue;

        auto results = findString(snapshot, targetString);
        EXPECT_LE(results.size(), 1);
    }

    tp.stop();
}
