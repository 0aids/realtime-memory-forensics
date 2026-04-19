#include <gtest/gtest.h>
#include <memory>
#include "logger.hpp"
#include "memory_graph.hpp"
#include "types.hpp"

using namespace rmf::graph;

TEST(StructRegistryTest, RegisterSimpleStruct)
{
    StructRegistry registry;

    auto           id = registry.registerr("SimpleStruct")
                  .field("int*", "x")
                  .field("int", "y")
                  .end();

    EXPECT_NE(id, StructRegistry::BAD_ID);
    EXPECT_TRUE(registry.containsParentId(id));
}

TEST(StructRegistryTest, RegisterMultipleStructsUniqueIds)
{
    rmf::g_logLevel = rmf_Verbose;
    StructRegistry registry;

    auto id1 = registry.registerr("StructA").field("int", "a").end();
    auto id2 = registry.registerr("StructB").field("int", "b").end();
    auto id3 =
        registry.registerr("StructC").field("float", "c").end();

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

    auto id = registry.registerr("MyStruct").field("char", "x").end();

    auto foundId = registry.getParentId("MyStruct");
    EXPECT_TRUE(foundId.has_value());
    EXPECT_EQ(foundId.value(), id);
}

TEST(StructRegistryTest, GetParentIdByNameNotFound)
{
    StructRegistry registry;

    registry.registerr("Existing").field("int", "x").end();

    auto foundId = registry.getParentId("NonExistent");
    EXPECT_FALSE(foundId.has_value());
}

TEST(StructRegistryTest, SingleFieldOffsetAndAlignment)
{
    StructRegistry registry;

    auto           id =
        registry.registerr("SingleField").field("int", "x").end();

    StructMemberId memberId{id, 0};

    EXPECT_TRUE(registry.containsFieldId(memberId));

    auto offset = registry.getFieldOffset(memberId);
    EXPECT_TRUE(offset.has_value());
    EXPECT_EQ(offset.value(), 0);

    auto alignment = registry.getFieldAlignmentRules(memberId);
    EXPECT_TRUE(alignment.has_value());
    EXPECT_EQ(alignment.value().alignedAs, sizeof(int));
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
                  .field("int", "a")
                  .field("char", "b")
                  .field("int", "c")
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
        double b;
        char   c;
        int    d;
    };

    auto id = registry.registerr("MultiField")
                  .field("double", "a")
                  .field("double", "b")
                  .field("char", "c")
                  .field("int", "d")
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
                  .field("char", "x")
                  .field("int", "y")
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
        registry.registerr("Valid").field("int", "x").end();
    auto invalidId = static_cast<StructTypeId>(999);

    EXPECT_TRUE(registry.containsParentId(validId));
    EXPECT_FALSE(registry.containsParentId(invalidId));
}

TEST(StructRegistryTest, GetFieldsOfParent)
{
    StructRegistry registry;

    auto           id = registry.registerr("TestStruct")
                  .field("int", "a")
                  .field("int", "b")
                  .field("float", "c")
                  .end();

    auto fields = registry.getFieldsOfParent(id);

    EXPECT_TRUE(fields.has_value());
    EXPECT_EQ(fields.value().size(), 3);
}

TEST(StructRegistryTest, GetFieldsOfParentInvalidId)
{
    StructRegistry registry;

    registry.registerr("Valid").field("char", "x").end();

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
                  .field("int", "x")
                  .field("double", "y")
                  .end();

    auto rules = registry.getStructAlignmentRules(id);

    EXPECT_TRUE(rules.has_value());
    EXPECT_EQ(rules.value().alignedAs, alignof(AlignedStruct));
    EXPECT_EQ(rules.value().totalSize, sizeof(AlignedStruct));
}

TEST(StructRegistryTest, GetStructAlignmentRulesByName)
{
    StructRegistry registry;

    registry.registerr("AlignedStruct")
        .field("double", "x")
        .field("int", "y")
        .end();

    auto rules = registry.getStructAlignmentRules("AlignedStruct");

    EXPECT_TRUE(rules.has_value());
    EXPECT_EQ(rules.value().alignedAs, alignof(double));
}

TEST(StructRegistryTest, GetStructAlignmentRulesNotFound)
{
    StructRegistry registry;

    registry.registerr("Existing").field("double", "x").end();

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
                  .field("char", "charField")
                  .field("int", "intField")
                  .field("double", "doubleField")
                  .end();

    StructMemberId charMember{id, 0};
    StructMemberId intMember{id, 1};
    StructMemberId doubleMember{id, 2};

    auto charAlign   = registry.getFieldAlignmentRules(charMember);
    auto intAlign    = registry.getFieldAlignmentRules(intMember);
    auto doubleAlign = registry.getFieldAlignmentRules(doubleMember);

    EXPECT_TRUE(charAlign.has_value());
    EXPECT_TRUE(intAlign.has_value());
    EXPECT_TRUE(doubleAlign.has_value());

    EXPECT_EQ(charAlign.value().alignedAs, alignof(char));
    EXPECT_EQ(intAlign.value().alignedAs, alignof(int));
    EXPECT_EQ(doubleAlign.value().alignedAs, alignof(double));
}

TEST(StructRegistryTest, GetFieldAlignmentInvalid)
{
    StructRegistry registry;

    auto           id =
        registry.registerr("TestStruct").field("double", "x").end();

    StructMemberId invalid{id, 99};

    auto alignment = registry.getFieldAlignmentRules(invalid);
    EXPECT_FALSE(alignment.has_value());
}

TEST(StructRegistryTest, NestedStruct)
{
    StructRegistry registry;

    registry.registerr("Inner").field("int", "value").end();
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
                       .field("Inner", "inner")
                       .field("int", "extra")
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
        .field("int", "a")
        .field("int", "b")
        .end();

    auto outerId =
        registry.registerr("Outer").field("Inner", "inner").end();

    auto rules = registry.getStructAlignmentRules(outerId);
    EXPECT_TRUE(rules.has_value());
    EXPECT_GE(rules.value().alignedAs, alignof(int));
}

TEST(StructRegistryTest, UnknownTypeSilentlyIgnored)
{
    StructRegistry registry;

    auto           id = registry.registerr("TestStruct")
                  .field("int", "known")
                  .field("UnknownType", "unknown1")
                  .field("float", "known2")
                  .field("AnotherUnknown", "unknown2")
                  .end();

    auto fields = registry.getFieldsOfParent(id);
    EXPECT_TRUE(fields.has_value());
    EXPECT_EQ(fields.value().size(), 2);
}

TEST(StructRegistryTest, PointerTypes)
{
    StructRegistry registry;

    auto           id = registry.registerr("PtrStruct")
                  .field("int*", "intPtr")
                  .field("void*", "voidPtr")
                  .end();

    auto fields = registry.getFieldsOfParent(id);
    EXPECT_TRUE(fields.has_value());
    EXPECT_GE(fields.value().size(), 1);

    auto alignment =
        registry.getFieldAlignmentRules(fields.value()["voidPtr"]);
    EXPECT_TRUE(alignment.has_value());
    EXPECT_EQ(alignment.value().alignedAs, sizeof(void*));
}

TEST(StructRegistryTest, AllBasicTypes)
{
    StructRegistry registry;

    auto           id = registry.registerr("AllBasicTypes")
                  .field("uint64_t", "u64")
                  .field("bool", "b")
                  .field("char", "c")
                  .field("short", "s")
                  .field("int", "i")
                  .field("long", "l")
                  .field("float", "f")
                  .field("double", "d")
                  .field("size_t", "sz")
                  .field("int8_t", "i8")
                  .field("uint8_t", "u8")
                  .field("int16_t", "i16")
                  .field("uint16_t", "u16")
                  .field("int32_t", "i32")
                  .field("uint32_t", "u32")
                  .field("int64_t", "i64")
                  .end();

    auto fields = registry.getFieldsOfParent(id);
    EXPECT_TRUE(fields.has_value());
    EXPECT_GE(fields.value().size(), 15);
}

TEST(StructRegistryTest, RestructureMrpBasic)
{
    StructRegistry registry;

    auto           id = registry.registerr("TestStruct")
                  .field("uint64_t", "x")
                  .field("int", "y")
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
                  .field("int", "x")
                  .field("char", "y")
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
                  .field("int", "a")
                  .field("int", "b")
                  .field("int", "c")
                  .field("int", "d")
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
        registry.registerr("MoveTest1").field("int", "x").end();
    auto id2 =
        registry.registerr("MoveTest2").field("int", "y").end();

    EXPECT_NE(id1, id2);
    EXPECT_TRUE(registry.containsParentId(id1));
    EXPECT_TRUE(registry.containsParentId(id2));
}

TEST(StructRegistryTest, GetFieldOffsetInvalid)
{
    StructRegistry registry;

    auto           id =
        registry.registerr("TestStruct").field("float", "x").end();

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
                  .field("int", "value")
                  .field("int", "next")
                  .end();

    EXPECT_NE(id, StructRegistry::BAD_ID);

    auto foundId = graph.structRegistry.getParentId("GraphStruct");
    EXPECT_TRUE(foundId.has_value());
    EXPECT_EQ(foundId.value(), id);

    auto rules = graph.structRegistry.getStructAlignmentRules(id);
    EXPECT_TRUE(rules.has_value());
}

TEST(StructRegistryTest, CanPointToSelf)
{
    MemoryGraphData graph;

    auto            id = graph.structRegistry.registerr("Node")
                  .field("uint32_t", "data")
                  .field("Node*", "next")
                  .end();
    struct Node
    {
        uint32_t data;
        Node*    next;
    };
    Node                 n1 = {0xf0f0f0f0, nullptr};
    Node                 n2 = {0xf0f0f0f0, &n1};
    std::vector<uint8_t> bytes(sizeof(n2), 16);
    memcpy(bytes.data(), &n2, sizeof(n2));
    const auto nodePointerId =
        graph.structRegistry.getFieldOfParent(id, "next").value();
    const auto nodeDataId =
        graph.structRegistry.getFieldOfParent(id, "data").value();
    // stupid runtime types.
    {
        const auto received =
            graph.structRegistry
                .getValuesAtMember(bytes, nodePointerId)
                .value();
        EXPECT_EQ(received.size(), sizeof(Node*));
        EXPECT_EQ(*(Node**)received.data(), n2.next);
    }
    {
        const auto received =
            graph.structRegistry.getValuesAtMember(bytes, nodeDataId)
                .value();
        EXPECT_EQ(received.size(), sizeof(uint32_t));
        EXPECT_EQ(*(uint32_t*)received.data(), n2.data);
    }
}

TEST(StructRegistryTest, ProperlyResizesMrp)
{
    MemoryGraphData graph;

    auto            id = graph.structRegistry.registerr("Node")
                  .field("uint32_t", "data")
                  .field("Node*", "next")
                  .end();
    auto fieldNode =
        graph.structRegistry.getFieldOfParent(id, "next").value();
    struct Node
    {
        uint32_t data;
        Node*    next;
    };
    rmf::types::MemoryRegionProperties mrpOfNodeStar = {
        .parentRegionAddress   = 0x0,
        .parentRegionSize      = 0x1000,
        .relativeRegionAddress = 0x38,
        .relativeRegionSize    = 0x8,
        .regionName_sp = std::make_shared<std::string>("bruh"),
        .perms = rmf::types::Perms::Read | rmf::types::Perms::Write,
    };
    rmf::types::MemoryRegionProperties correctedMrp = {
        .parentRegionAddress   = 0x0,
        .parentRegionSize      = 0x1000,
        .relativeRegionAddress = 0x30,
        .relativeRegionSize    = sizeof(Node),
        .regionName_sp = std::make_shared<std::string>("bruh"),
        .perms = rmf::types::Perms::Read | rmf::types::Perms::Write,
    };
    auto newMrp =
        graph.structRegistry.restructureMrp(fieldNode, mrpOfNodeStar);
    EXPECT_EQ(newMrp, correctedMrp);
}
