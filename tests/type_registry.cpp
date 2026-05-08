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

// Beautiful macro magic.
#define MAKE_STRUCTMember(type, name, ...) type name;

#define MakeField(type, name, ...) .field(#type, #name)

// BUG: doesn't work with arrays because the length is associated
// with the name, so getting the offset is incorrect.
#define ASSERT_SAME_OFFSET(type, fieldName, StructName, RegisteredStruct, ...) \
    EXPECT_EQ(offsetof(StructName, fieldName),                                 \
              RegisteredStruct[#fieldName].offset());

#define MAKE_STRUCT(StructName, DefinitionMacro)                               \
    struct StructName                                                          \
    {                                                                          \
        DefinitionMacro(MAKE_STRUCTMember, StructName)                         \
    }

#define MAKE_REGISTERED_TYPE(TypeRegistry, StructName, DefinitionMacro)        \
    TypeRegistry.addStruct(StructName) DefinitionMacro(MakeField, StructName)  \
        .end()

#define TEST_STRUCT_DEFINITION(X, StructName, ...)                             \
    X(uint32_t, data, __VA_ARGS__)                                             \
    X(uint8_t, array[4], __VA_ARGS__)                                          \
    X(StructName*, next, __VA_ARGS__)

TEST(type_registry, registerTest)
{
    GTEST_SKIP_("TODO");
    // Setting up
    MAKE_STRUCT(TestStruct, TEST_STRUCT_DEFINITION);

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

    // ACTUAL START!!!

    mf::TypeRegistry tr;

    mf::Struct       testStruct =
        MAKE_REGISTERED_TYPE(tr, "TestStruct", TEST_STRUCT_DEFINITION);

    //     // Ensure that the offsets are all the same.
    //     // TEST_STRUCT_DEFINITION(ASSERT_SAME_OFFSET, TestStruct, TestStruct,
    //     //                        testStruct);

    //     // Can use operator[] for unchecked access.
    //     // Will throw if invalid however.
    //     mf::Field testStructField = testStruct.field("data").value();

    //     mf::Node<mf::Map, mf::Snapshot, mf::Struct> coerced =
    //         testStructField.reshapeNode(std::move(snap));

    //     // How nice, no lambda wrapping required.
    //     EXPECT_ANY_THROW(mf::Field invalidField =
    //                          testStruct["i don't exist"]);
}
