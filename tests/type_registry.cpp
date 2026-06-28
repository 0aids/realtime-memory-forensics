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
    EXPECT_GT(i, 0);
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
}

TEST(type_registry, PrimitiveV0)
{
    auto tr = rmf::TypeRegistry::Make();
    EXPECT_EQ(tr.prim.v0.size(), 0);
    EXPECT_EQ(tr.prim.v0.alignment(), 0);
    EXPECT_EQ(tr.prim.v0.type(), rmf::Type::Primitive);
    EXPECT_EQ(tr.prim.v0.name(), "v0");
}

// -------------------------------------------------------------------
// Struct layout
// -------------------------------------------------------------------

TEST(type_registry, EmptyStruct)
{
    auto tr = rmf::TypeRegistry::Make();
    auto s  = tr.defStruct("Empty").end();
    EXPECT_EQ(s.size(), 0);
    EXPECT_EQ(s.alignment(), 0);
    EXPECT_EQ(s.begin(), s.end());
}

TEST(type_registry, SingleField)
{
    auto tr = rmf::TypeRegistry::Make();
    auto s  = tr.defStruct("S").field(tr.prim.u32, "x").end();
    EXPECT_EQ(s.size(), 4);
    EXPECT_EQ(s.alignment(), 4);
    auto f = s.getField("x");
    ASSERT_TRUE(f.has_value());
    EXPECT_EQ(f->offset(), 0);
}

struct PaddingLayout
{
    uint8_t  a;
    uint64_t b;
};

TEST(type_registry, PaddingRequired)
{
    auto tr = rmf::TypeRegistry::Make();
    auto s =
        tr.defStruct("P").field(tr.prim.u8, "a").field(tr.prim.u64, "b").end();
    EXPECT_EQ(s.size(), sizeof(PaddingLayout));
    EXPECT_EQ(s.alignment(), alignof(PaddingLayout));
    EXPECT_EQ(s.getField("a")->offset(), offsetof(PaddingLayout, a));
    EXPECT_EQ(s.getField("b")->offset(), offsetof(PaddingLayout, b));
}

struct NoPaddingLayout
{
    uint32_t x;
    uint32_t y;
};

TEST(type_registry, NoPadding)
{
    auto tr = rmf::TypeRegistry::Make();
    auto s =
        tr.defStruct("N").field(tr.prim.u32, "x").field(tr.prim.u32, "y").end();
    EXPECT_EQ(s.size(), sizeof(NoPaddingLayout));
    EXPECT_EQ(s.alignment(), alignof(NoPaddingLayout));
    EXPECT_EQ(s.getField("x")->offset(), offsetof(NoPaddingLayout, x));
    EXPECT_EQ(s.getField("y")->offset(), offsetof(NoPaddingLayout, y));
}

struct MultiPaddingLayout
{
    uint8_t  a;
    uint16_t b;
    uint32_t c;
    uint64_t d;
};

TEST(type_registry, MultiplePadding)
{
    auto tr = rmf::TypeRegistry::Make();
    auto s  = tr.defStruct("M")
                  .field(tr.prim.u8, "a")
                  .field(tr.prim.u16, "b")
                  .field(tr.prim.u32, "c")
                  .field(tr.prim.u64, "d")
                  .end();
    EXPECT_EQ(s.size(), sizeof(MultiPaddingLayout));
    EXPECT_EQ(s.alignment(), alignof(MultiPaddingLayout));
    EXPECT_EQ(s.getField("a")->offset(), offsetof(MultiPaddingLayout, a));
    EXPECT_EQ(s.getField("b")->offset(), offsetof(MultiPaddingLayout, b));
    EXPECT_EQ(s.getField("c")->offset(), offsetof(MultiPaddingLayout, c));
    EXPECT_EQ(s.getField("d")->offset(), offsetof(MultiPaddingLayout, d));
}

// -------------------------------------------------------------------
// Field access
// -------------------------------------------------------------------

TEST(type_registry, GetFieldMissing)
{
    auto tr = rmf::TypeRegistry::Make();
    auto s  = tr.defStruct("S").field(tr.prim.u32, "x").end();
    EXPECT_FALSE(s.getField("does_not_exist").has_value());
}

TEST(type_registry, BracketExisting)
{
    auto tr = rmf::TypeRegistry::Make();
    auto s  = tr.defStruct("S").field(tr.prim.u32, "x").end();
    auto f  = s["x"];
    EXPECT_EQ(f.name(), "x");
    EXPECT_EQ(f.offset(), 0);
}

TEST(type_registry, BracketMissingThrows)
{
    auto tr = rmf::TypeRegistry::Make();
    auto s  = tr.defStruct("S").field(tr.prim.u32, "x").end();
    EXPECT_THROW(s["nope"], std::runtime_error);
}

// -------------------------------------------------------------------
// containsField
// -------------------------------------------------------------------

TEST(type_registry, ContainsExisting)
{
    auto tr = rmf::TypeRegistry::Make();
    auto s  = tr.defStruct("S").field(tr.prim.u32, "x").end();
    EXPECT_TRUE(s.containsField("x"));
}

TEST(type_registry, ContainsMissing)
{
    auto tr = rmf::TypeRegistry::Make();
    auto s  = tr.defStruct("S").field(tr.prim.u32, "x").end();
    EXPECT_FALSE(s.containsField("nope"));
}

TEST(type_registry, ContainsByField)
{
    auto tr = rmf::TypeRegistry::Make();
    auto s  = tr.defStruct("S").field(tr.prim.u32, "x").end();
    auto f  = s["x"];
    EXPECT_TRUE(s.containsField(f));
}

// -------------------------------------------------------------------
// Iterator
// -------------------------------------------------------------------

TEST(type_registry, EmptyStructEndEqualsBegin)
{
    auto tr = rmf::TypeRegistry::Make();
    auto s  = tr.defStruct("E").end();
    EXPECT_EQ(s.begin(), s.end());
}

TEST(type_registry, IteratorPostfix)
{
    auto tr = rmf::TypeRegistry::Make();
    auto s =
        tr.defStruct("S").field(tr.prim.u32, "a").field(tr.prim.u32, "b").end();
    auto it  = s.begin();
    auto old = it++;
    EXPECT_NE(old, it);
    EXPECT_NE(it, s.end());
    ++it;
    EXPECT_EQ(it, s.end());
}

TEST(type_registry, StdDistance)
{
    auto tr = rmf::TypeRegistry::Make();
    auto s  = tr.defStruct("S")
                  .field(tr.prim.u32, "a")
                  .field(tr.prim.u16, "b")
                  .field(tr.prim.u8, "c")
                  .end();
    EXPECT_EQ(std::distance(s.begin(), s.end()), 3);
}

// -------------------------------------------------------------------
// arrOf
// -------------------------------------------------------------------

TEST(type_registry, ArrayOfPrimitive)
{
    auto tr = rmf::TypeRegistry::Make();
    auto a  = tr.arrOf(tr.prim.u32, 5);
    EXPECT_EQ(a.name(), "u32[5]");
    EXPECT_EQ(a.size(), 20);
    EXPECT_EQ(a.alignment(), 4);
    EXPECT_EQ(a.type(), rmf::Type::Array);
}

TEST(type_registry, ArraySizeZero)
{
    auto tr = rmf::TypeRegistry::Make();
    auto a  = tr.arrOf(tr.prim.u32, 0);
    EXPECT_EQ(a.name(), "u32[0]");
    EXPECT_EQ(a.size(), 0);
}

TEST(type_registry, ArrayOfArray)
{
    auto tr    = rmf::TypeRegistry::Make();
    auto inner = tr.arrOf(tr.prim.u8, 3);
    auto outer = tr.arrOf(inner, 2);
    EXPECT_EQ(outer.name(), "u8[3][2]");
    EXPECT_EQ(outer.size(), 6);
    EXPECT_EQ(outer.alignment(), 1);
}

// -------------------------------------------------------------------
// ptrTo
// -------------------------------------------------------------------

TEST(type_registry, PointerToPrimitive)
{
    auto tr = rmf::TypeRegistry::Make();
    auto p  = tr.ptrTo(tr.prim.u32);
    EXPECT_EQ(p.name(), "u32*");
    EXPECT_EQ(p.size(), sizeof(void*));
    EXPECT_EQ(p.alignment(), sizeof(void*));
    EXPECT_EQ(p.type(), rmf::Type::Pointer);
}

TEST(type_registry, PointerToSelf)
{
    auto tr = rmf::TypeRegistry::Make();
    tr.defStruct("MyStruct").field(tr.prim.u8, "x").end();
    auto p = tr.ptrTo(tr.struct_("MyStruct"));
    EXPECT_EQ(p.name(), "MyStruct*");
}

TEST(type_registry, PointerToPointer)
{
    auto tr = rmf::TypeRegistry::Make();
    auto p  = tr.ptrTo(tr.ptrTo(tr.prim.u8));
    EXPECT_EQ(p.name(), "u8**");
}

// -------------------------------------------------------------------
// struct_ lookup
// -------------------------------------------------------------------

TEST(type_registry, LookupExisting)
{
    auto tr = rmf::TypeRegistry::Make();
    tr.defStruct("A").field(tr.prim.u32, "x").end();
    auto s = tr.struct_("A");
    EXPECT_EQ(s.name(), "A");
    EXPECT_TRUE(s.containsField("x"));
}

TEST(type_registry, LookupMissingThrows)
{
    auto tr = rmf::TypeRegistry::Make();
    EXPECT_THROW(tr.struct_("DoesNotExist"), std::out_of_range);
}

// -------------------------------------------------------------------
// Multiple structs
// -------------------------------------------------------------------

struct StructALayout
{
    uint32_t x;
};
struct StructBLayout
{
    uint64_t y;
};

TEST(type_registry, TwoStructs)
{
    auto tr = rmf::TypeRegistry::Make();
    auto a  = tr.defStruct("A").field(tr.prim.u32, "x").end();
    auto b  = tr.defStruct("B").field(tr.prim.u64, "y").end();
    EXPECT_EQ(a.size(), sizeof(StructALayout));
    EXPECT_EQ(b.size(), sizeof(StructBLayout));
    EXPECT_EQ(a.getField("x")->offset(), offsetof(StructALayout, x));
    EXPECT_EQ(b.getField("y")->offset(), offsetof(StructBLayout, y));
}
