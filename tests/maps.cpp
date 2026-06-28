#include <gtest/gtest.h>
#include "helpers.hpp"
#include <rmf/maps.hpp>

// -------------------------------------------------------------------
// Map address boundaries
// -------------------------------------------------------------------

TEST(map, PbeginPend)
{
    rmf::Map m{
        .name  = std::make_shared<const std::string>("t"),
        .pAddr = 0x1000,
        .pSize = 0x100,
        .rAddr = 0x10,
        .rSize = 0x20,
    };
    EXPECT_EQ(m.pbegin(), 0x1000);
    EXPECT_EQ(m.pend(), 0x1100);
}

TEST(map, RbeginRend)
{
    rmf::Map m{
        .name  = std::make_shared<const std::string>("t"),
        .pAddr = 0x1000,
        .pSize = 0x100,
        .rAddr = 0x10,
        .rSize = 0x20,
    };
    EXPECT_EQ(m.rbegin(), 0x10);
    EXPECT_EQ(m.rend(), 0x30);
}

TEST(map, TbeginTend)
{
    rmf::Map m{
        .name  = std::make_shared<const std::string>("t"),
        .pAddr = 0x1000,
        .pSize = 0x100,
        .rAddr = 0x10,
        .rSize = 0x20,
    };
    EXPECT_EQ(m.tbegin(), 0x1010);
    EXPECT_EQ(m.tend(), 0x1030);
}

TEST(map, ZeroSize)
{
    rmf::Map m{};
    EXPECT_EQ(m.pbegin(), m.pend());
    EXPECT_EQ(m.rbegin(), m.rend());
    EXPECT_EQ(m.tbegin(), m.tend());
}

TEST(map, NegativeRAddr)
{
    rmf::Map m{
        .name  = std::make_shared<const std::string>("t"),
        .pAddr = 0x1000,
        .pSize = 0x100,
        .rAddr = -8,
        .rSize = 8,
    };
    EXPECT_EQ(m.tbegin(), 0x1000 - 8);
    EXPECT_EQ(m.tend(), 0x1000);
    EXPECT_EQ(m.rbegin(), static_cast<uintptr_t>(-8));
    EXPECT_EQ(m.rend(), 0);
}

TEST(map, Addresses)
{
    rmf::Map m{nullptr, 0x1000, 0x100, 0x10, 0x20};
    EXPECT_EQ(m.pbegin(), 0x1000);
    EXPECT_EQ(m.pend(), 0x1100);
    EXPECT_EQ(m.rbegin(), 0x10);
    EXPECT_EQ(m.rend(), 0x30);
    EXPECT_EQ(m.tbegin(), 0x1010);
    EXPECT_EQ(m.tend(), 0x1030);
}

// -------------------------------------------------------------------
// Map::valid
// -------------------------------------------------------------------

TEST(map, ValidWithName)
{
    rmf::Map m{
        .name = std::make_shared<const std::string>("x"),
    };
    EXPECT_TRUE(m.valid());
}

TEST(map, ValidDefault)
{
    rmf::Map m;
    EXPECT_FALSE(m.valid());
}

// -------------------------------------------------------------------
// Perms
// -------------------------------------------------------------------

TEST(perms, OrOperator)
{
    rmf::Perms a(rmf::Perms::Read);
    rmf::Perms b(rmf::Perms::Write);
    rmf::Perms p = a | b;
    EXPECT_TRUE((bool)(p & rmf::Perms::Read));
    EXPECT_TRUE((bool)(p & rmf::Perms::Write));
}

TEST(perms, OrAssign)
{
    rmf::Perms p(rmf::Perms::Read);
    p |= rmf::Perms::Execute;
    EXPECT_TRUE((bool)(p & rmf::Perms::Read));
    EXPECT_TRUE((bool)(p & rmf::Perms::Execute));
}

TEST(perms, ParseEmpty)
{
    auto p = rmf::Perms_Parse(std::string_view{});
    EXPECT_EQ(p, rmf::Perms::None);
}

TEST(perms, ParseRWX)
{
    auto p = rmf::Perms_Parse(std::string_view("rwx"));
    EXPECT_TRUE((bool)(p & rmf::Perms::Read));
    EXPECT_TRUE((bool)(p & rmf::Perms::Write));
    EXPECT_TRUE((bool)(p & rmf::Perms::Execute));
}

TEST(perms, ParseShared)
{
    auto p = rmf::Perms_Parse(std::string_view("rws"));
    EXPECT_TRUE((bool)(p & rmf::Perms::Read));
    EXPECT_TRUE((bool)(p & rmf::Perms::Write));
    EXPECT_TRUE((bool)(p & rmf::Perms::Shared));
}

TEST(perms, ParseUppercase)
{
    auto low  = rmf::Perms_Parse(std::string_view("rwx"));
    auto high = rmf::Perms_Parse(std::string_view("RWX"));
    EXPECT_EQ(low, high);
}

TEST(perms, ParsePartial)
{
    rmf::Perms p = rmf::Perms_Parse(std::string_view("r--"));
    EXPECT_TRUE((bool)(p & rmf::Perms::Read));
    EXPECT_FALSE((bool)(p & rmf::Perms::Write));
    EXPECT_FALSE((bool)(p & rmf::Perms::Execute));
}

TEST(perms, ParseMixedCase)
{
    auto p = rmf::Perms_Parse(std::string_view("RwX"));
    EXPECT_TRUE((bool)(p & rmf::Perms::Read));
    EXPECT_TRUE((bool)(p & rmf::Perms::Write));
    EXPECT_TRUE((bool)(p & rmf::Perms::Execute));
}

TEST(perms, Equality)
{
    auto a = rmf::Perms_Parse(std::string_view("rw"));
    auto b = rmf::Perms_Parse(std::string_view("rw"));
    auto c = rmf::Perms_Parse(std::string_view("rx"));
    EXPECT_TRUE(a == b);
    EXPECT_TRUE(a != c);
}

// -------------------------------------------------------------------
// Map::chunkify
// -------------------------------------------------------------------

TEST(chunkify, ExactFit)
{
    rmf::Map m{
        .name  = std::make_shared<const std::string>("t"),
        .pAddr = 0,
        .pSize = 100,
        .rAddr = 0,
        .rSize = 100,
    };
    auto chunks = m.chunkify(10);
    ASSERT_EQ(chunks.size(), 10);
    for (size_t i = 0; i < 10; ++i)
    {
        EXPECT_EQ(chunks[i].rAddr, static_cast<ptrdiff_t>(i * 10));
        EXPECT_EQ(chunks[i].rSize, 10);
    }
}

TEST(chunkify, WithRemainder)
{
    rmf::Map m{
        .name  = std::make_shared<const std::string>("t"),
        .pAddr = 0,
        .pSize = 10,
        .rAddr = 0,
        .rSize = 10,
    };
    auto chunks = m.chunkify(3);
    ASSERT_EQ(chunks.size(), 4);
    EXPECT_EQ(chunks[0].rAddr, 0);
    EXPECT_EQ(chunks[0].rSize, 3);
    EXPECT_EQ(chunks[1].rAddr, 3);
    EXPECT_EQ(chunks[1].rSize, 3);
    EXPECT_EQ(chunks[2].rAddr, 6);
    EXPECT_EQ(chunks[2].rSize, 3);
    EXPECT_EQ(chunks[3].rAddr, 9);
    EXPECT_EQ(chunks[3].rSize, 1);
}

TEST(chunkify, SingleChunk)
{
    rmf::Map m{
        .name  = std::make_shared<const std::string>("t"),
        .pAddr = 0,
        .pSize = 5,
        .rAddr = 0,
        .rSize = 5,
    };
    auto chunks = m.chunkify(100);
    ASSERT_EQ(chunks.size(), 1);
    EXPECT_EQ(chunks[0].rAddr, 0);
    EXPECT_EQ(chunks[0].rSize, 5);
}

TEST(chunkify, ZeroSize)
{
    rmf::Map m{
        .name  = std::make_shared<const std::string>("t"),
        .pAddr = 0,
        .pSize = 0,
        .rAddr = 0,
        .rSize = 0,
    };
    auto chunks = m.chunkify(10);
    EXPECT_TRUE(chunks.empty());
}

TEST(chunkify, NonZeroRAddr)
{
    rmf::Map m{
        .name  = std::make_shared<const std::string>("t"),
        .pAddr = 0,
        .pSize = 50,
        .rAddr = 100,
        .rSize = 50,
    };
    auto chunks = m.chunkify(10);
    ASSERT_EQ(chunks.size(), 5);
    EXPECT_EQ(chunks[0].rAddr, 100);
    EXPECT_EQ(chunks[0].rSize, 10);
    EXPECT_EQ(chunks[4].rAddr, 140);
    EXPECT_EQ(chunks[4].rSize, 10);
}

TEST(chunkify, OverlapZero)
{
    rmf::Map m{
        .name  = std::make_shared<const std::string>("t"),
        .pAddr = 0,
        .pSize = 10,
        .rAddr = 0,
        .rSize = 10,
    };
    auto chunks = m.chunkify(5, 0);
    ASSERT_EQ(chunks.size(), 2);
    EXPECT_EQ(chunks[0].rAddr, 0);
    EXPECT_EQ(chunks[0].rSize, 5);
    EXPECT_EQ(chunks[1].rAddr, 5);
    EXPECT_EQ(chunks[1].rSize, 5);
}

TEST(chunkify, OverlapNonZero)
{
    rmf::Map m{
        .name  = std::make_shared<const std::string>("t"),
        .pAddr = 0,
        .pSize = 10,
        .rAddr = 0,
        .rSize = 10,
    };
    auto chunks = m.chunkify(5, 2);
    ASSERT_EQ(chunks.size(), 3);
    EXPECT_EQ(chunks[0].rAddr, 0);
    EXPECT_EQ(chunks[0].rSize, 5);
    EXPECT_EQ(chunks[1].rAddr, 3);
    EXPECT_EQ(chunks[1].rSize, 5);
    EXPECT_EQ(chunks[2].rAddr, 6);
    EXPECT_EQ(chunks[2].rSize, 4);
}

TEST(chunkify, OverlapPartialLast)
{
    rmf::Map m{
        .name  = std::make_shared<const std::string>("t"),
        .pAddr = 0,
        .pSize = 7,
        .rAddr = 0,
        .rSize = 7,
    };
    auto chunks = m.chunkify(3, 1);
    ASSERT_EQ(chunks.size(), 3);
    EXPECT_EQ(chunks[0].rAddr, 0);
    EXPECT_EQ(chunks[0].rSize, 3);
    EXPECT_EQ(chunks[1].rAddr, 2);
    EXPECT_EQ(chunks[1].rSize, 3);
    EXPECT_EQ(chunks[2].rAddr, 4);
    EXPECT_EQ(chunks[2].rSize, 3);
}
