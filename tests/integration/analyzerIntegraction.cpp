#include <gtest/gtest.h>
#include <rmf.hpp>
#include <ranges>
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

TEST(analyzerIntegrationTest, findInt32)
{
    using namespace rmf::test;

    volatile int32_t targetValue = 0x99998888;
    testProcess      tp;
    tp.build<staticValueComponent>(targetValue);
    pid_t childPid = tp.run();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::string mapsPath =
        "/proc/" + std::to_string(childPid) + "/maps";
    auto          regions         = ParseMaps(mapsPath);
    auto          readableRegions = regions.FilterHasPerms("r");
    rmf::Analyzer analyzer(6);

    ASSERT_FALSE(readableRegions.empty());

    MemoryRegionPropertiesVec flattened;
    {
        rmf_Log(rmf_Info,
                "Finding target value: " << std::hex << std::showbase
                                         << targetValue);
        auto snaps =
            analyzer.Execute(MemorySnapshot::Make, regions, childPid);

        // Should find itself and others
        auto result = analyzer.Execute(findNumeralExact<int32_t>,
                                       snaps, targetValue) |
            std::views::join;
        std::move(result.begin(), result.end(),
                  std::back_inserter(flattened));
        rmf_Log(rmf_Info,
                "Found target value " << flattened.size()
                                      << " times!");
        EXPECT_TRUE(flattened.size() >= 2);
    }

    tp.stop();
}

TEST(analyzerIntegrationTest, findInt32AndPtrs)
{
    using namespace rmf::test;

    volatile int32_t targetValue = 0x88776655;
    testProcess      tp;
    tp.build<staticValueComponent>(targetValue);
    pid_t childPid = tp.run();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::string mapsPath =
        "/proc/" + std::to_string(childPid) + "/maps";
    auto regions = ParseMaps(mapsPath);
    auto readableRegions =
        regions.FilterHasPerms("r").FilterActiveRegions(childPid);
    rmf::Analyzer analyzer(6);

    ASSERT_FALSE(readableRegions.empty());

    auto snaps =
        analyzer.Execute(MemorySnapshot::Make, regions, childPid);
    MemoryRegionPropertiesVec flattened;
    {
        rmf_Log(rmf_Info,
                "Finding target value: " << std::hex << std::showbase
                                         << targetValue);
        // Should find itself and others
        auto result = analyzer.Execute(findNumeralExact<int32_t>,
                                       snaps, targetValue) |
            std::views::join;
        std::move(result.begin(), result.end(),
                  std::back_inserter(flattened));
        rmf_Log(rmf_Info,
                "Found target value " << flattened.size()
                                      << " times!");
        EXPECT_TRUE(flattened.size() >= 2);
    }

    {
        // Should find itself and others
        rmf_Log(rmf_Info, "Finding pointers!");
        auto result = analyzer.Execute(findPointersToRegions, snaps,
                                       flattened) |
            std::views::join;
        flattened.clear();
        std::move(result.begin(), result.end(),
                  std::back_inserter(flattened));
        rmf_Log(rmf_Info,
                "Found pointers " << flattened.size() << " times!");
        EXPECT_TRUE(flattened.size() >= 2);
    }

    tp.stop();
}
