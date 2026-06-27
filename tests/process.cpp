#include <gtest/gtest.h>
#include <rmf/process.hpp>
#include <rmf/maps.hpp>
#include "helpers.hpp"
#include <vector>

TEST(process, ReadMaps)
{
    auto forker = ForkedProcess(infiniteLoopFunc);
    auto proc   = rmf::Process(forker.pid);
    auto maps   = proc.getMaps<std::vector>();
    EXPECT_GT(maps.size(), 0);
}
