#include <cstdint>
#include <gtest/gtest.h>
#include <rmf/maps.hpp>
#include <rmf/snapshots.hpp>
#define RMF_NO_CLEANUP_MACROS
#include <rmf/type_registry.hpp>

using namespace std;
using namespace std::literals;

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

    auto                 tr = rmf::TypeRegistry::Make();
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

    // Will throw if invalid however.
    rmf::Field testStructField = testStruct.getField("data").value();

    // Invalid test
    EXPECT_FALSE(testStruct.getField("I don't exist!").has_value());

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
    auto tr = rmf::TypeRegistry::Make();
#define X(_name, _size)                                                        \
    EXPECT_EQ(tr.prim._name##_size.name(), #_name #_size);                     \
    EXPECT_EQ(tr.prim._name##_size.alignment(), _size / 8);                    \
    EXPECT_EQ(tr.prim._name##_size.size(), _size / 8);                         \
    EXPECT_EQ(tr.prim._name##_size.type(), rmf::Type::Primitive);

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
