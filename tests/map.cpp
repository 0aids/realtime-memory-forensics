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
    println("{}", test1.regionName_sp.use_count());
    println("{}", std::string(test1));
    EXPECT_EQ(test1, test2);
}
