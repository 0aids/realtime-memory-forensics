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
namespace mf  = rmf;
namespace mfl = mf::Logging;
namespace mfu = mf::Utils;
namespace mft = mf::Tests;
using namespace mf;

TEST(node, templateAddingWorksAsExpected)
{
    using NodeMap = Node<Map>;
    static_assert(std::same_as<NodeMap, NodeMap::AddFeature<Map>>);
}
