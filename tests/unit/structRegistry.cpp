#include <gtest/gtest.h>
#include "memory_graph.hpp"

using namespace rmf::graph;

TEST(StructRegistryTest, RegisterSimpleStruct)
{
    StructRegistry registry;

    auto           id = registry.registerr("SimpleStruct")
                  .field("x", "int")
                  .field("y", "int")
                  .end();

    EXPECT_NE(id, StructRegistry::BAD_ID);
    EXPECT_TRUE(registry.containsParentId(id));
}

TEST(StructRegistryTest, RegisterMultipleStructsUniqueIds)
{
    StructRegistry registry;

    auto id1 = registry.registerr("StructA").field("a", "int").end();
    auto id2 =
        registry.registerr("StructB").field("b", "float").end();
    auto id3 = registry.registerr("StructC").field("c", "char").end();

    EXPECT_NE(id1, id2);
    EXPECT_NE(id2, id3);
    EXPECT_NE(id1, id3);

    EXPECT_TRUE(registry.containsParentId(id1));
    EXPECT_TRUE(registry.containsParentId(id2));
    EXPECT_TRUE(registry.containsParentId(id3));
}

TEST(StructRegistryTest, GetParentIdByName)
{
    StructRegistry registry;

    auto id = registry.registerr("MyStruct").field("x", "int").end();

    auto foundId = registry.getParentId("MyStruct");
    EXPECT_TRUE(foundId.has_value());
    EXPECT_EQ(foundId.value(), id);
}

TEST(StructRegistryTest, GetParentIdByNameNotFound)
{
    StructRegistry registry;

    registry.registerr("Existing").field("x", "int").end();

    auto foundId = registry.getParentId("NonExistent");
    EXPECT_FALSE(foundId.has_value());
}

TEST(StructRegistryTest, SingleFieldOffsetAndAlignment)
{
    StructRegistry registry;

    auto           id =
        registry.registerr("SingleField").field("x", "int").end();

    StructMemberId memberId{id, 0};

    EXPECT_TRUE(registry.containsFieldId(memberId));

    auto offset = registry.getFieldOffset(memberId);
    EXPECT_TRUE(offset.has_value());
    EXPECT_EQ(offset.value(), 0);

    auto alignment = registry.getFieldAlignment(memberId);
    EXPECT_TRUE(alignment.has_value());
    EXPECT_EQ(alignment.value(), sizeof(int));
}

TEST(StructRegistryTest, MultipleFieldsOffsets)
{
    StructRegistry registry;
    struct multiField
    {
        char   a;
        int    b;
        double c;
    };

    auto id = registry.registerr("MultiField")
                  .field("a", "char")
                  .field("b", "int")
                  .field("c", "double")
                  .end();

    auto memberIds = registry.getFieldsOfParent(id).value();
    auto offsetA   = registry.getFieldOffset(memberIds.at("a"));
    auto offsetB   = registry.getFieldOffset(memberIds.at("b"));
    auto offsetC   = registry.getFieldOffset(memberIds.at("c"));

    EXPECT_TRUE(offsetA.has_value());
    EXPECT_TRUE(offsetB.has_value());
    EXPECT_TRUE(offsetC.has_value());

    EXPECT_EQ(offsetA.value(), offsetof(multiField, a));
    EXPECT_EQ(offsetB.value(), offsetof(multiField, b));
    EXPECT_EQ(offsetC.value(), offsetof(multiField, c));
}

TEST(StructRegistryTest, MultipleFieldsOffsets2)
{
    StructRegistry registry;
    struct multiField
    {
        double a;
        char   b;
        int    c;
        char   d;
    };

    auto id = registry.registerr("MultiField")
                  .field("a", "double")
                  .field("b", "char")
                  .field("c", "int")
                  .field("d", "char")
                  .end();

    auto memberIds = registry.getFieldsOfParent(id).value();
    auto offsetA   = registry.getFieldOffset(memberIds["a"]);
    auto offsetB   = registry.getFieldOffset(memberIds["b"]);
    auto offsetC   = registry.getFieldOffset(memberIds["c"]);
    auto offsetD   = registry.getFieldOffset(memberIds["d"]);

    EXPECT_TRUE(offsetA.has_value());
    EXPECT_TRUE(offsetB.has_value());
    EXPECT_TRUE(offsetC.has_value());
    EXPECT_TRUE(offsetD.has_value());

    EXPECT_EQ(offsetA.value(), offsetof(multiField, a));
    EXPECT_EQ(offsetB.value(), offsetof(multiField, b));
    EXPECT_EQ(offsetC.value(), offsetof(multiField, c));
    EXPECT_EQ(offsetD.value(), offsetof(multiField, d));
    EXPECT_EQ(sizeof(multiField),
              registry.getStructAlignmentRules(id).value().totalSize);
}

TEST(StructRegistryTest, ContainsFieldIdValidInvalid)
{
    StructRegistry registry;

    auto           id = registry.registerr("TestStruct")
                  .field("x", "int")
                  .field("y", "int")
                  .end();

    StructMemberId valid{id, 0};
    StructMemberId valid2{id, 1};
    StructMemberId invalidIndex{id, 2};
    StructMemberId invalidType{999, 0};

    EXPECT_TRUE(registry.containsFieldId(valid));
    EXPECT_TRUE(registry.containsFieldId(valid2));
    EXPECT_FALSE(registry.containsFieldId(invalidIndex));
    EXPECT_FALSE(registry.containsFieldId(invalidType));
}

TEST(StructRegistryTest, ContainsParentIdValidInvalid)
{
    StructRegistry registry;

    auto           validId =
        registry.registerr("Valid").field("x", "int").end();
    auto invalidId = static_cast<StructTypeId>(999);

    EXPECT_TRUE(registry.containsParentId(validId));
    EXPECT_FALSE(registry.containsParentId(invalidId));
}

TEST(StructRegistryTest, GetFieldsOfParent)
{
    StructRegistry registry;

    auto           id = registry.registerr("TestStruct")
                  .field("a", "int")
                  .field("b", "float")
                  .field("c", "char")
                  .end();

    auto fields = registry.getFieldsOfParent(id);

    EXPECT_TRUE(fields.has_value());
    EXPECT_EQ(fields.value().size(), 3);
}

TEST(StructRegistryTest, GetFieldsOfParentInvalidId)
{
    StructRegistry registry;

    registry.registerr("Valid").field("x", "int").end();

    auto fields = registry.getFieldsOfParent(999);
    EXPECT_FALSE(fields.has_value());
}

TEST(StructRegistryTest, GetStructAlignmentRulesById)
{
    StructRegistry registry;
    struct AlignedStruct
    {
        int    x;
        double y;
    };

    auto id = registry.registerr("AlignedStruct")
                  .field("x", "int")
                  .field("y", "double")
                  .end();

    auto rules = registry.getStructAlignmentRules(id);

    EXPECT_TRUE(rules.has_value());
    EXPECT_EQ(rules.value().alignedAs, alignof(double));
    EXPECT_EQ(rules.value().totalSize, sizeof(AlignedStruct));
}

TEST(StructRegistryTest, GetStructAlignmentRulesByName)
{
    StructRegistry registry;

    registry.registerr("AlignedStruct")
        .field("x", "int")
        .field("y", "double")
        .end();

    auto rules = registry.getStructAlignmentRules("AlignedStruct");

    EXPECT_TRUE(rules.has_value());
    EXPECT_EQ(rules.value().alignedAs, alignof(double));
}

TEST(StructRegistryTest, GetStructAlignmentRulesNotFound)
{
    StructRegistry registry;

    registry.registerr("Existing").field("x", "int").end();

    auto rulesById = registry.getStructAlignmentRules(
        static_cast<StructTypeId>(999));
    auto rulesByName =
        registry.getStructAlignmentRules("NonExistent");

    EXPECT_FALSE(rulesById.has_value());
    EXPECT_FALSE(rulesByName.has_value());
}

TEST(StructRegistryTest, GetFieldAlignment)
{
    StructRegistry registry;

    auto           id = registry.registerr("TestStruct")
                  .field("charField", "char")
                  .field("intField", "int")
                  .field("doubleField", "double")
                  .end();

    StructMemberId charMember{id, 0};
    StructMemberId intMember{id, 1};
    StructMemberId doubleMember{id, 2};

    auto           charAlign = registry.getFieldAlignment(charMember);
    auto           intAlign  = registry.getFieldAlignment(intMember);
    auto doubleAlign = registry.getFieldAlignment(doubleMember);

    EXPECT_TRUE(charAlign.has_value());
    EXPECT_TRUE(intAlign.has_value());
    EXPECT_TRUE(doubleAlign.has_value());

    EXPECT_EQ(charAlign.value(), alignof(char));
    EXPECT_EQ(intAlign.value(), alignof(int));
    EXPECT_EQ(doubleAlign.value(), alignof(double));
}

TEST(StructRegistryTest, GetFieldAlignmentInvalid)
{
    StructRegistry registry;

    auto           id =
        registry.registerr("TestStruct").field("x", "int").end();

    StructMemberId invalid{id, 99};

    auto           alignment = registry.getFieldAlignment(invalid);
    EXPECT_FALSE(alignment.has_value());
}

TEST(StructRegistryTest, NestedStruct)
{
    StructRegistry registry;

    registry.registerr("Inner").field("value", "int").end();
    struct Inner
    {
        int value;
    };
    struct Outer
    {
        Inner inner;
        int   extra;
    };

    auto outerId = registry.registerr("Outer")
                       .field("inner", "Inner")
                       .field("extra", "int")
                       .end();

    auto outerRules = registry.getStructAlignmentRules(outerId);
    EXPECT_TRUE(outerRules.has_value());

    auto fields = registry.getFieldsOfParent(outerId);
    EXPECT_TRUE(fields.has_value());
    EXPECT_EQ(fields.value().size(), 2);
    auto f = fields.value();

    EXPECT_EQ(registry.getParentOfField(f.at("inner")).has_value(),
              true);
    EXPECT_EQ(registry.getFieldOffset(f.at("extra")).value(),
              offsetof(Outer, extra));
    EXPECT_EQ(registry.getFieldOffset(f.at("inner")).value(),
              offsetof(Outer, inner));
    EXPECT_EQ(
        registry.getStructAlignmentRules(outerId).value().totalSize,
        sizeof(Outer));
}

TEST(StructRegistryTest, NestedStructAlignment)
{
    StructRegistry registry;

    registry.registerr("Inner")
        .field("a", "int")
        .field("b", "int")
        .end();

    auto outerId =
        registry.registerr("Outer").field("inner", "Inner").end();

    auto rules = registry.getStructAlignmentRules(outerId);
    EXPECT_TRUE(rules.has_value());
    EXPECT_GE(rules.value().alignedAs, alignof(int));
}

TEST(StructRegistryTest, UnknownTypeSilentlyIgnored)
{
    StructRegistry registry;

    auto           id = registry.registerr("TestStruct")
                  .field("known", "int")
                  .field("unknown1", "UnknownType")
                  .field("known2", "float")
                  .field("unknown2", "AnotherUnknown")
                  .end();

    auto fields = registry.getFieldsOfParent(id);
    EXPECT_TRUE(fields.has_value());
    EXPECT_EQ(fields.value().size(), 2);
}

TEST(StructRegistryTest, PointerTypes)
{
    StructRegistry registry;

    auto           id = registry.registerr("PtrStruct")
                  .field("intPtr", "int*")
                  .field("voidPtr", "void*")
                  .end();

    auto fields = registry.getFieldsOfParent(id);
    EXPECT_TRUE(fields.has_value());
    EXPECT_GE(fields.value().size(), 1);

    auto alignment =
        registry.getFieldAlignment(fields.value()["voidPtr"]);
    EXPECT_TRUE(alignment.has_value());
    EXPECT_EQ(alignment.value(), sizeof(void*));
}

TEST(StructRegistryTest, AllBasicTypes)
{
    StructRegistry registry;

    auto           id = registry.registerr("AllBasicTypes")
                  .field("b", "bool")
                  .field("c", "char")
                  .field("s", "short")
                  .field("i", "int")
                  .field("l", "long")
                  .field("ll", "long long")
                  .field("f", "float")
                  .field("d", "double")
                  .field("u", "unsigned int")
                  .field("ul", "unsigned long")
                  .field("ull", "unsigned long long")
                  .field("sz", "size_t")
                  .field("i8", "int8_t")
                  .field("u8", "uint8_t")
                  .field("i16", "int16_t")
                  .field("u16", "uint16_t")
                  .field("i32", "int32_t")
                  .field("u32", "uint32_t")
                  .field("i64", "int64_t")
                  .field("u64", "uint64_t")
                  .end();

    auto fields = registry.getFieldsOfParent(id);
    EXPECT_TRUE(fields.has_value());
    EXPECT_GE(fields.value().size(), 15);
}

TEST(StructRegistryTest, RestructureMrpBasic)
{
    StructRegistry registry;

    auto           id = registry.registerr("TestStruct")
                  .field("x", "int")
                  .field("y", "int")
                  .end();

    StructMemberId                     root{id, 0};

    rmf::types::MemoryRegionProperties mrp;
    mrp.parentRegionAddress   = 0x10000;
    mrp.relativeRegionAddress = 0x100;
    mrp.relativeRegionSize    = 0x10;
    mrp.regionName_sp = std::make_shared<const std::string>("test");

    auto newMrp = registry.restructureMrp(root, mrp);

    EXPECT_EQ(newMrp.parentRegionAddress, mrp.parentRegionAddress);
}

TEST(StructRegistryTest, RestructureMrpWithOffset)
{
    StructRegistry registry;

    auto           id = registry.registerr("TestStruct")
                  .field("x", "char")
                  .field("y", "int")
                  .end();

    StructMemberId                     root{id, 1};

    rmf::types::MemoryRegionProperties mrp;
    mrp.parentRegionAddress   = 0x10000;
    mrp.relativeRegionAddress = 0x200;
    mrp.relativeRegionSize    = 0x100;
    mrp.regionName_sp = std::make_shared<const std::string>("test");

    auto offset = registry.getFieldOffset(root);
    ASSERT_TRUE(offset.has_value());
    EXPECT_GT(offset.value(), 0);

    auto newMrp = registry.restructureMrp(root, mrp);

    EXPECT_EQ(newMrp.parentRegionAddress, mrp.parentRegionAddress);
}

TEST(StructRegistryTest, RestructureMrpSizeIncrease)
{
    StructRegistry registry;

    auto           id = registry.registerr("LargeStruct")
                  .field("a", "int")
                  .field("b", "int")
                  .field("c", "int")
                  .field("d", "int")
                  .end();

    StructMemberId                     root{id, 0};

    rmf::types::MemoryRegionProperties mrp;
    mrp.parentRegionAddress   = 0x10000;
    mrp.relativeRegionAddress = 0x100;
    mrp.relativeRegionSize    = 4;
    mrp.regionName_sp = std::make_shared<const std::string>("test");

    auto newMrp = registry.restructureMrp(root, mrp);

    EXPECT_EQ(newMrp.parentRegionAddress, mrp.parentRegionAddress);
}

TEST(StructRegistryTest, EmptyStruct)
{
    StructRegistry registry;

    auto           id = registry.registerr("EmptyStruct").end();

    EXPECT_NE(id, StructRegistry::BAD_ID);

    auto fields = registry.getFieldsOfParent(id);
    EXPECT_TRUE(fields.has_value());
    EXPECT_TRUE(fields.value().empty());

    auto rules = registry.getStructAlignmentRules(id);
    EXPECT_TRUE(rules.has_value());
}

TEST(StructRegistryTest, StructBuilderMoveSemantics)
{
    StructRegistry registry;

    auto           id1 =
        registry.registerr("MoveTest1").field("x", "int").end();
    auto id2 =
        registry.registerr("MoveTest2").field("y", "float").end();

    EXPECT_NE(id1, id2);
    EXPECT_TRUE(registry.containsParentId(id1));
    EXPECT_TRUE(registry.containsParentId(id2));
}

TEST(StructRegistryTest, GetFieldOffsetInvalid)
{
    StructRegistry registry;

    auto           id =
        registry.registerr("TestStruct").field("x", "int").end();

    StructMemberId invalidIndex{id, 99};
    StructMemberId invalidType{999, 0};

    auto offsetIndex = registry.getFieldOffset(invalidIndex);
    auto offsetType  = registry.getFieldOffset(invalidType);

    EXPECT_FALSE(offsetIndex.has_value());
    EXPECT_FALSE(offsetType.has_value());
}

TEST(StructRegistryTest, MemoryGraphDataStructRegistryIntegration)
{
    MemoryGraphData graph;

    auto            id = graph.structRegistry.registerr("GraphStruct")
                  .field("value", "int")
                  .field("next", "int*")
                  .end();

    EXPECT_NE(id, StructRegistry::BAD_ID);

    auto foundId = graph.structRegistry.getParentId("GraphStruct");
    EXPECT_TRUE(foundId.has_value());
    EXPECT_EQ(foundId.value(), id);

    auto rules = graph.structRegistry.getStructAlignmentRules(id);
    EXPECT_TRUE(rules.has_value());
}
