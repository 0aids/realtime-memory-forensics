#include "gtest/gtest.h"
#include <cstdint>
#include <gtest/gtest.h>
#include <print>
#include <rmf/logging/logging.hpp>
#include <rmf/rmf.hpp>
#include <rmf/utils/expect.hpp>
#include <rmf/utils/str.hpp>
#include <rmf/node.hpp>
#include <rmf/map.hpp>
#include <rmf/snapshot.hpp>
#include "helpers.hpp"
#include <rmf/type_registry.hpp>
#include "rmf/test_helpers.hpp"
#include "rmf/utils/function.hpp"
#include "rmf/op.hpp"
#include "rmf/utils/threadpool.hpp"

using namespace std;
namespace mf  = RealtimeMemoryForensics;
namespace mfl = mf::Logging;
namespace mfu = mf::Utils;
namespace mft = mf::Tests;

TEST(type_registry, registerTest)
{
    // Setting up
    struct TestStruct
    {
        uint32_t    data;
        uint8_t     array[4];
        TestStruct* next;
    };

    auto tr         = mf::TypeRegistry::Make();
    auto testStruct = tr.defStruct("TestStruct")
                          .field(tr.prim.u32, "data")
                          .field(tr.arrOf(tr.prim.u8, 4), "array")
                          .field(tr.ptrTo(tr.struct_("TestStruct")), "next")
                          .end();

    GTEST_SKIP_("TODO");

    TestStruct t = {
        .data  = 0xaabbccdd,
        .array = {0xaa, 0xbb, 0xcc, 0xdd},
        .next  = nullptr,
    };
    t.next = &t;

    // Make it reference itself? This will work if we set the
    // map to be from 0x00.
    auto buffer = mf::Tests::TestBuffer::makeZeroed(sizeof(TestStruct));

    EXPECT_TRUE((bool)buffer.pushUnaligned(t));

    mf::Node<mf::Map, mf::Snapshot> snap =
        mf::Snapshot::fromBuffer(buffer.moveBuffer());

    // Can use operator[] for unchecked access.
    // Will throw if invalid however.
    mf::Field testStructField = testStruct.getField("data").value();

    mf::Node<mf::Map, mf::Snapshot, mf::Typed> coerced =
        testStructField.nodify(std::move(snap));
}
