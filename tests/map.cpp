#include "rmf/snapshot.hpp"
#include <gtest/gtest.h>
#include <print>
#include <rmf/logging/logging.hpp>
#include <rmf/rmf.hpp>
#include <rmf/utils/expect.hpp>
#include <rmf/utils/str.hpp>
#include <rmf/node.hpp>
#include <rmf/map.hpp>
#include <rmf/test_helpers.hpp>
#include <rmf/op.hpp>

using namespace std;
namespace mf  = RealtimeMemoryForensics;
namespace mfl = mf::Logging;
namespace mfu = mf::Utils;
namespace mft = mf::Tests;

TEST(map, BasicRegion)
{
    mf::Node<mf::Map> test1;
    mf::Node<mf::Map> test2;
    println("{}", test1.map.regionName_sp.use_count());
    println("{}", std::string(test1));
    EXPECT_EQ(test1.map, test2.map);
}

TEST(map, MapSnapConversion)
{
    auto name = std::make_shared<const std::string>("hello world");
    mf::Detail::MapData data = {
        .parentAddress   = 0xff,
        .parentSize      = 0xffff,
        .relativeAddress = 0xaa,
        .relativeSize    = 0xff,
        .regionName_sp   = name,
    };
    mf::Node<mf::Map, mf::Snapshot> test1;
    test1.map = data;
    mf::Node<mf::Map> test2(test1);
    // What in the holy heck.
    EXPECT_EQ(test1.map, test2.map);
}

TEST(map, SnapMapConversion)
{
    auto name = std::make_shared<const std::string>("hello world");
    mf::Detail::MapData data = {
        .parentAddress   = 0xff,
        .parentSize      = 0xffff,
        .relativeAddress = 0xaa,
        .relativeSize    = 0xff,
        .regionName_sp   = name,
    };
    mf::Node<mf::Map>               test1;
    mf::Node<mf::Map, mf::Snapshot> test2(test1);
    EXPECT_EQ(test1.map, test2.map);
}

TEST(map, mapInformationOperations)
{
    auto name = std::make_shared<const std::string>("hello world");
    mf::Detail::MapData data = {
        .parentAddress   = 0xff,
        .parentSize      = 0xffff,
        .relativeAddress = 0xaa,
        .relativeSize    = 0xff,
        .regionName_sp   = name,
    };
    mf::Node<mf::Map> test1;
    test1.map = data;
    EXPECT_EQ(test1.pbegin(), data.parentAddress);
    EXPECT_EQ(test1.pend(), data.parentAddress + data.parentSize);
    EXPECT_EQ(test1.tbegin(),
              data.parentAddress + data.relativeAddress);
    EXPECT_EQ(test1.tend(),
              data.parentAddress + data.relativeAddress +
                  data.relativeSize);
    EXPECT_EQ(test1.rbegin(), data.relativeAddress);
    EXPECT_EQ(test1.rend(), data.relativeAddress + data.relativeSize);
}
TEST(map, testProgramReading)
{
    using namespace mft;
    pid_t                       pid  = forkFunc(createTestProgram(
        StaticNumberBuffer<int, 0xfafaf>(), TestFeature{},
        StaticStringBuffer{.buffer = "hello world"}));
    mfu::Vec<mf::Node<mf::Map>> maps = mf::getMaps(pid);
    EXPECT_GE(maps.size(), 0);
    for (const auto& map : maps)
    {
        println("Map: {}", std::string(map));
    }
    // Attempt to filter
}
