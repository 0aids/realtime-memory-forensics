#include <gtest/gtest.h>
#include <print>
#include <rmf/process.hpp>
#include <rmf/maps.hpp>
#include "helpers.hpp"
#include <vector>

TEST(process, ReadMaps)
{
    auto forker = ForkedProcess(infiniteLoopFunc);
    auto proc   = rmf::Process(forker.pid);
    auto maps   = proc.getMaps();
    EXPECT_GT(maps.size(), 0);
}

TEST(process, ReadMemory)
{
    auto forker = ForkedProcess(infiniteLoopFunc);
    auto proc   = rmf::Process(forker.pid);
    auto maps   = proc.getMaps();
    EXPECT_GT(maps.size(), 0);
    auto snaps = proc.getSnapshots(maps);
    EXPECT_EQ(snaps.size(), maps.size());
    std::println("snaps: {}, maps: {}", snaps.size(), maps.size());
    std::span<uint8_t> firstSnapSpan = *snaps[0].data;
    std::println("snaps first 100 bytes: {::02x}",
                 firstSnapSpan.subspan(0, 100));
}
