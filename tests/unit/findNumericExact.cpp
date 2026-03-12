#include <gtest/gtest.h>
#include <iterator>
#include <thread>
#include <chrono>
#include <cstring>
#include "logger.hpp"
#include "test_helpers.hpp"
#include "utils.hpp"
#include "types.hpp"
#include "operations.hpp"

using namespace rmf::utils;
using namespace rmf::types;
using namespace rmf::op;

// constexpr int32_t int32DefaultValue = 0x99998888;

TEST(findNumericExactTest, findInt32InTestProcessDefault)
{
    using namespace rmf::test;

    volatile int32_t     targetValue = 0x99998888;
    uint8_t     numFound    = 0;
    testProcess tp;
    tp.build<staticValueComponent>();
    pid_t childPid = tp.run();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::string mapsPath =
        "/proc/" + std::to_string(childPid) + "/maps";
    auto regions         = ParseMaps(mapsPath, childPid);
    auto readableRegions = regions.FilterHasPerms("r");

    ASSERT_FALSE(readableRegions.empty());

    // Should find itself and others

    for (const auto& mrp : readableRegions)
    {
        auto snapshot = MemorySnapshot::Make(mrp);
        if (!snapshot.isValid())
            continue;

        auto results =
            findNumeralExact<int32_t>(snapshot, targetValue);
        if (!results.empty())
        {
            numFound += results.size();
            continue;
        }
    }

    EXPECT_TRUE(numFound >= 2);
    tp.stop();
}

TEST(findNumericExactTest, findInt32InTestProcess)
{
    using namespace rmf::test;

    volatile int32_t     targetValue       = 0x12345678;
    uint32_t    numFound          = 0;
    uint32_t    defaultValueFound = 0;
    testProcess tp;
    tp.build<staticValueComponent>(targetValue);
    pid_t childPid = tp.run();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::string mapsPath =
        "/proc/" + std::to_string(childPid) + "/maps";
    auto regions         = ParseMaps(mapsPath, childPid);
    auto readableRegions = regions.FilterHasPerms("r");
    MemoryRegionPropertiesVec foundRegions;

    ASSERT_FALSE(readableRegions.empty());

    // Should find itself and others

    for (const auto& mrp : readableRegions)
    {
        auto snapshot = MemorySnapshot::Make(mrp);
        if (!snapshot.isValid())
            continue;

        auto results =
            findNumeralExact<int32_t>(snapshot, targetValue);
        if (!results.empty())
        {
            numFound += results.size();

            std::move(results.begin(), results.end(),
                      std::back_inserter(foundRegions));
            continue;
        }
        // auto resultsDefault =
        //     findNumeralExact<int32_t>(snapshot, int32DefaultValue);
        // if (!resultsDefault.empty())
        // {
        //     defaultValueFound += resultsDefault.size();
        //     break;
        // }
    }
    rmf_Log(rmf_Info, "numFound: " << numFound);
    // rmf_Log(rmf_Info, "defaultValueFound: " <<defaultValueFound);

    // bool notTheSame = false;

    // for (const auto& mrp : foundRegions)
    // {
    //     rmf_Log(rmf_Info,
    //             "target address: "
    //                 << &targetValue << ", found address: " << std::hex
    //                 << std::showbase << mrp.TrueAddress());
    //     if (mrp.TrueAddress() != (uintptr_t)&targetValue)
    //         notTheSame = true;
    // }

    EXPECT_TRUE(numFound >= 2);
    tp.stop();
}

TEST(findNumericExactTest, findInt64InTestProcess)
{
    using namespace rmf::test;

    testProcess tp;
    int64_t     targetValue = 0xfedcba9876543210;
    tp.build<staticValueComponent>(0, targetValue);
    pid_t childPid = tp.run();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::string mapsPath =
        "/proc/" + std::to_string(childPid) + "/maps";
    auto regions         = ParseMaps(mapsPath, childPid);
    auto readableRegions = regions.FilterHasPerms("r");

    ASSERT_FALSE(readableRegions.empty());

    uint32_t numFound = 0;

    for (const auto& mrp : readableRegions)
    {
        auto snapshot = MemorySnapshot::Make(mrp);
        if (!snapshot.isValid())
            continue;

        auto results =
            findNumeralExact<int64_t>(snapshot, targetValue);
        if (!results.empty())
        {
            numFound += results.size();
            continue;
        }
    }
    rmf_Log(rmf_Info, "numFound: " << numFound);

    EXPECT_TRUE(numFound >= 2);
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

    // There should always be one existing here because
    // the entire process' memory is copied, which mean the forked process will
    // see this.
    int64_t targetValue = 0x1234567891234567;

    for (const auto& mrp : readableRegions)
    {
        auto snapshot = MemorySnapshot::Make(mrp);
        if (!snapshot.isValid())
            continue;

        auto results =
            findNumeralExact<int64_t>(snapshot, targetValue);
        EXPECT_TRUE(results.size() <= 1);
    }

    tp.stop();
}
