#include "gtest/gtest.h"
#include <cstdint>
#include <gtest/gtest.h>
#include <print>
#include <rmf/logging/logging.hpp>
#include <rmf/utils/expect.hpp>
#include <rmf/utils/str.hpp>
#include <rmf/node.hpp>
#include <rmf/map.hpp>
#include <rmf/snapshot.hpp>
#include "rmf/test_helpers.hpp"
#include "rmf/utils/function.hpp"
#include "rmf/op.hpp"
#include "rmf/utils/threadpool.hpp"
#define RMF_NO_CLEANUP_MACROS
#include <rmf/type_registry.hpp>

using namespace std;
namespace mf  = rmf;
namespace mfl = mf::Logging;
namespace mfu = mf::Utils;
namespace mft = mf::Tests;

// Setting up
struct TestStruct
{
    uint32_t    data;
    uint8_t     array[4];
    TestStruct* next;
};

TEST(type_registry, registerTest)
{

    std::vector<ssize_t> offsets = {
        offsetof(TestStruct, data),
        offsetof(TestStruct, array),
        offsetof(TestStruct, next),
    };
    std::vector<ssize_t> sizes = {};

    auto                 tr = mf::TypeRegistry::Make();
    auto testStruct = tr.defStruct("TestStruct")
                          .field(tr.prim.u32, "data")
                          .field(tr.arrOf(tr.prim.u8, 4), "array")
                          .field(tr.ptrTo(tr.struct_("TestStruct")), "next")
                          .end();

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

    // Will throw if invalid however.
    mf::Field testStructField = testStruct.getField("data").value();

    // Invalid test
    EXPECT_ANY_THROW(testStruct.getField("I don't exist!").value());

    // Ensure size is correct.
    EXPECT_EQ(testStruct.size(), sizeof(TestStruct));
    // Iterate through all the fields, and compare them against the actual values.
    size_t i = 0;

    // No reference iteration.
    for (const auto field : testStruct)
    {
        EXPECT_EQ(field.offset(), offsets[i++]);
    }
}

TEST(type_registry, EnsurePrimitiveTypesConstructed)
{
    auto tr = mf::TypeRegistry::Make();
#define X(_name, _size)                                                        \
    EXPECT_EQ(tr.prim._name##_size.name(), #_name #_size);                     \
    EXPECT_EQ(tr.prim._name##_size.alignment(), _size / 8);                    \
    EXPECT_EQ(tr.prim._name##_size.size(), _size / 8);                         \
    EXPECT_EQ(tr.prim._name##_size.type(), mf::Type::Primitive);

    RMF_PRIM_TYPES(X);
#undef X
    struct Test
    {
        int      a;
        uint64_t d;
        char     b;
        int      c;
        int      e;
    };
}
constexpr std::array<uint8_t, 4> defaultArray = {0xaa, 0xbb, 0xcc, 0xdd};

TEST(type_registry, NodificationTest1)
{
    auto tr         = mf::TypeRegistry::Make();
    auto testStruct = tr.defStruct("TestStruct")
                          .field(tr.prim.u32, "data")
                          .field(tr.arrOf(tr.prim.u8, 4), "array")
                          .field(tr.ptrTo(tr.struct_("TestStruct")), "next")
                          .end();
    TestStruct t    = {
        .data  = 0xaabbccdd,
        .array = {0xaa, 0xbb, 0xcc, 0xdd},
        .next  = nullptr,
    };

    // Make it reference itself? This will work if we set the
    // map to be from 0x00.
    t.next = &t;

    auto buffer = mf::Tests::TestBuffer::makeZeroed(sizeof(TestStruct));

    auto dataField = testStruct.getField("data").value();

    EXPECT_TRUE((bool)buffer.pushUnaligned(t));

    mf::Node<mf::Map, mf::Snapshot> snap =
        mf::Snapshot::fromBuffer(buffer.moveBuffer());

    mf::Node<mf::Map, mf::Snapshot, mf::Struct> res = testStruct.nodify(snap);
    auto bytes = res.bytesAtField(dataField);
    // Because x86 is little endian, 0xaabbccdd is stored as [dd cc bb aa], with smallest value first;
    EXPECT_EQ(*reinterpret_cast<uint32_t*>(bytes.data()), 0xddccbbaa);
    auto array = res.bytesAtField(testStruct.getField("array").value());

    for (size_t i = 0; i < 4; i++)
    {
        EXPECT_EQ(array[i], defaultArray[4 - i - 1]);
    }
}

// Attempt to follow a pointer chain.
TEST(type_registry, pointerFollowing)
{
    using namespace mft;
    using namespace mf;
    pid_t pid       = forkFunc(createTestProgram(SinglyLinkedListFeature<5>{}));
    TypeRegistry tr = TypeRegistry::Make();

    // find the first.
    mfu::ThreadPool tp(thread::hardware_concurrency() / 2);
    // BUG: snapshot, map not convertible from map, snapshot?
    mfu::Vec<Node<Map, Snapshot>> maps = getMaps<Snapshot>(pid).hasPerms("r");

    // Make snapshots, find string "hello world! 00".
    mfu::Vec<Node<Map, Snapshot>> result =
        maps.mapThreaded<Snapshot::captureM>(pid).with(tp);
    auto helloworlds = findString.threaded(result, "hello world! 00").with(tp);
    EXPECT_GE(helloworlds.size(), 1);
    println("Num hello worlds: {}", helloworlds.size());
    auto LinkedList_tr = tr.defStruct("LinkedList")
                             .field(tr.arrOf(tr.prim.u8, 100), "data")
                             .field(tr.ptrTo(tr.struct_("LinkedList")), "next")
                             .end();
    auto dataField     = LinkedList_tr.getField("data").value();
    auto pointerField  = LinkedList_tr.getField("next").value();
    for (const auto& hello : helloworlds)
    {
        // Attempt to get a node at a specified address
        auto nodified = LinkedList_tr.nodifyFromField(hello, dataField);
        // Holy crap how nice.
        // But what about if it fails? It should fail silently and get passed forward.
        auto derefPointer = nodified.fieldNode(pointerField)
                                .getTarget<Pointer>()
                                .targetNode<Struct>(maps);

        derefPointer.capture(pid);
    }
}
