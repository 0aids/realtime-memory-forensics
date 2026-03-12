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

TEST(findNumericChangedTest, detectChangedValue)
{
    using namespace rmf::test;

    testProcess tp;
    tp.build<incrementingIntComponent>(0, 1);
    pid_t childPid = tp.run();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::string mapsPath =
        "/proc/" + std::to_string(childPid) + "/maps";
    auto regions         = ParseMaps(mapsPath, childPid);
    auto readableRegions = regions.FilterHasPerms("rw");

    ASSERT_FALSE(readableRegions.empty());

    std::optional<MemorySnapshot> snap1;
    std::optional<MemorySnapshot> snap2;

    for (const auto& mrp : readableRegions)
    {
        auto snapshot = MemorySnapshot::Make(mrp, childPid);
        if (snapshot.isValid())
        {
            if (!snap1.has_value())
                snap1 = snapshot;
            else if (!snap2.has_value())
                snap2 = snapshot;

            if (snap1.has_value() && snap2.has_value())
                break;
        }
    }

    ASSERT_TRUE(snap1.has_value());
    ASSERT_TRUE(snap2.has_value());

    std::this_thread::sleep_for(std::chrono::seconds(2));

    auto snap1Updated = MemorySnapshot::Make(snap1->getMrp(), childPid);
    auto snap2Updated = MemorySnapshot::Make(snap2->getMrp(), childPid);

    auto changedResults =
        findNumericChanged<int32_t>(snap1Updated, snap2Updated, 0);

    tp.stop();
}

TEST(findNumericChangedTest, unchangedRegionsBetweenSnapshots)
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

    auto& mrp   = readableRegions.front();
    auto  snap1 = MemorySnapshot::Make(mrp, childPid);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    auto snap2 = MemorySnapshot::Make(mrp, childPid);

    ASSERT_TRUE(snap1.isValid());
    ASSERT_TRUE(snap2.isValid());

    auto unchangedResults = findUnchangedRegions(snap1, snap2, 0x100);

    EXPECT_GE(unchangedResults.size(), 1);

    tp.stop();
}
