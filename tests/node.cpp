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
#include <rmf/type_registry.hpp>

using namespace std;
namespace mf  = rmf;
namespace mfl = mf::Logging;
namespace mfu = mf::Utils;
namespace mft = mf::Tests;
using namespace mf;

TEST(node, templateAddingWorksAsExpected)
{
    using NodeMap = Node<Map>;
    static_assert(
        std::same_as<Node<Map, Snapshot>, NodeMap::WithFeature<Snapshot>>,
        "With feature should add a non-existant feature");
}

TEST(node, templateWithDuplicateWorks)
{
    using NodeMap = Node<Map>;
    static_assert(
        std::same_as<NodeMap::Features, NodeMap::WithFeature<Map>::Features>,
        "WithFeature with preexisting feature should be teh same.");
}

TEST(node, templateWithoutFeature)
{
    using NodeMap = Node<Map>;
    static_assert(
        std::same_as<NodeMap::Features,
                     Node<Map, Snapshot>::WithoutFeature<Snapshot>::Features>,
        "Without feature should be valid");
}

TEST(node, templateOrderingDoesntMatter)
{
    static_assert(std::same_as<Node<Snapshot, Map>::Features,
                               Node<Map, Snapshot>::Features>,
                  "<Map, Snapshot> vs <Snapshot, Map> Ordering doesn't matter");
}

TEST(node, templateExclusiveTypesAreSwapped)
{
    using typedNode = Node<Map, Snapshot, Typed>;
    static_assert(std::same_as<Node<Map, Snapshot, Struct>::Features,
                               typedNode::withType<Struct>::Features>,
                  "Types should remove and swap exclusive values");
}
