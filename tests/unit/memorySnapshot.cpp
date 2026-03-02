#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include "test_helpers.hpp"
#include "utils.hpp"
#include "types.hpp"

using namespace rmf::utils;
using namespace rmf::types;

TEST(memorySnapshotTest, takeSnapshotOfTestProcess)
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

    auto& firstRegion = readableRegions.front();
    auto  snapshot    = MemorySnapshot::Make(firstRegion);

    EXPECT_TRUE(snapshot.isValid());
    EXPECT_EQ(snapshot.getDataSpan().size(),
              firstRegion.relativeRegionSize);

    tp.stop();
}

TEST(memorySnapshotTest, snapshotDataMatchesExpected)
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

    for (const auto& mrp : readableRegions)
    {
        auto snapshot = MemorySnapshot::Make(mrp);
        if (snapshot.isValid())
        {
            EXPECT_EQ(snapshot.getDataSpan().size(),
                      mrp.relativeRegionSize);
        }
    }

    tp.stop();
}

TEST(memorySnapshotTest, multipleSnapshots)
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

    auto& region = readableRegions.front();

    auto  snapshot1 = MemorySnapshot::Make(region);
    auto  snapshot2 = MemorySnapshot::Make(region);

    EXPECT_TRUE(snapshot1.isValid());
    EXPECT_TRUE(snapshot2.isValid());

    EXPECT_EQ(snapshot1.getDataSpan().size(),
              snapshot2.getDataSpan().size());

    tp.stop();
}
