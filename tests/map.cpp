#include "rmf/snapshot.hpp"
#include <gtest/gtest.h>
#include <print>
#include <rmf/logging/logging.hpp>
#include <rmf/rmf.hpp>
#include <rmf/utils/expect.hpp>
#include <rmf/utils/str.hpp>
#include <rmf/node.hpp>
#include <rmf/map.hpp>

using namespace std;
namespace mf  = RealtimeMemoryForensics;
namespace mfl = mf::Logging;
namespace mfu = mf::Utils;
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
    mf::Node<mf::Map>               test2(test1);
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
