#include <gtest/gtest.h>
#include "helpers.hpp"
#include <rmf/op.hpp>

// -------------------------------------------------------------------
// findChanged
// -------------------------------------------------------------------

TEST(findChanged, Empty)
{
    auto a = makeMRV({});
    auto b = makeMRV({});
    auto r = findChanged(a, b, 1);
    EXPECT_TRUE(r.empty());
}

TEST(findChanged, Identical)
{
    auto a = makeMRV({1, 2, 3, 4});
    auto b = makeMRV({1, 2, 3, 4});
    auto r = findChanged(a, b, 1);
    EXPECT_TRUE(r.empty());
}

TEST(findChanged, AllDifferentCompareSize1)
{
    auto a = makeMRV({0, 0, 0, 0, 0, 0, 0, 0});
    auto b = makeMRV({1, 1, 1, 1, 1, 1, 1, 1});
    auto r = findChanged(a, b, 1);
    ASSERT_EQ(r.size(), 1);
    EXPECT_EQ(r[0].rAddr, 0);
    EXPECT_EQ(r[0].rSize, 8);
}

TEST(findChanged, CompareSizeMatches)
{
    auto a = makeMRV({0, 0, 0, 0, 0, 0, 0, 0});
    auto b = makeMRV({1, 1, 1, 1, 1, 1, 1, 1});
    auto r = findChanged(a, b, 8);
    ASSERT_EQ(r.size(), 1);
    EXPECT_EQ(r[0].rAddr, 0);
    EXPECT_EQ(r[0].rSize, 8);
}

TEST(findChanged, PartialMiddle)
{
    std::vector<uint8_t> a(12, 0);
    std::vector<uint8_t> b(12, 0);
    for (int i = 4; i < 8; ++i)
        b[i] = 1;

    auto r = findChanged(makeMRV(std::move(a)), makeMRV(std::move(b)), 1);
    ASSERT_EQ(r.size(), 1);
    EXPECT_EQ(r[0].rAddr, 4);
    EXPECT_EQ(r[0].rSize, 4);
}

TEST(findChanged, TwoSeparateRegions)
{
    std::vector<uint8_t> a(12, 0);
    std::vector<uint8_t> b(12, 0);
    b[0] = 1;
    b[1] = 1;
    b[8] = 1;
    b[9] = 1;

    auto r = findChanged(makeMRV(std::move(a)), makeMRV(std::move(b)), 1);
    ASSERT_EQ(r.size(), 2);
    EXPECT_EQ(r[0].rAddr, 0);
    EXPECT_EQ(r[0].rSize, 2);
    EXPECT_EQ(r[1].rAddr, 8);
    EXPECT_EQ(r[1].rSize, 2);
}

TEST(findChanged, AdjacentMerges)
{
    std::vector<uint8_t> a(8, 0);
    std::vector<uint8_t> b(8, 0);
    for (int i = 0; i < 4; ++i)
        b[i] = 1;

    auto r = findChanged(makeMRV(std::move(a)), makeMRV(std::move(b)), 2);
    ASSERT_EQ(r.size(), 1);
    EXPECT_EQ(r[0].rAddr, 0);
    EXPECT_EQ(r[0].rSize, 4);
}

TEST(findChanged, CompareSizePartialLast)
{
    std::vector<uint8_t> a(10, 0);
    std::vector<uint8_t> b(10, 0);
    for (int i = 0; i < 4; ++i)
        b[i] = 1;
    for (int i = 8; i < 10; ++i)
        b[i] = 1;

    auto r = findChanged(makeMRV(std::move(a)), makeMRV(std::move(b)), 4);
    ASSERT_EQ(r.size(), 2);
    EXPECT_EQ(r[0].rAddr, 0);
    EXPECT_EQ(r[0].rSize, 4);
    EXPECT_EQ(r[1].rAddr, 8);
    EXPECT_EQ(r[1].rSize, 2);
}

// -------------------------------------------------------------------
// findNumChanged
// -------------------------------------------------------------------

TEST(findNumChanged, NoChange)
{
    auto data1 = makeNumData<uint32_t>({0, 0, 0, 0});
    auto data2 = makeNumData<uint32_t>({0, 0, 0, 0});
    auto r     = findNumChanged(makeMRV(std::move(data1)),
                                makeMRV(std::move(data2)), uint32_t(1));
    EXPECT_TRUE(r.empty());
}

TEST(findNumChanged, BelowThreshold)
{
    auto data1 = makeNumData<uint32_t>({0});
    auto data2 = makeNumData<uint32_t>({3});
    auto r     = findNumChanged(makeMRV(std::move(data1)),
                                makeMRV(std::move(data2)), uint32_t(5));
    EXPECT_TRUE(r.empty());
}

TEST(findNumChanged, AtThreshold)
{
    auto data1 = makeNumData<uint32_t>({0});
    auto data2 = makeNumData<uint32_t>({5});
    auto r     = findNumChanged(makeMRV(std::move(data1)),
                                makeMRV(std::move(data2)), uint32_t(5));
    ASSERT_EQ(r.size(), 1);
    EXPECT_EQ(r[0].rAddr, 0);
    EXPECT_EQ(r[0].rSize, sizeof(uint32_t));
}

TEST(findNumChanged, AboveThreshold)
{
    auto data1 = makeNumData<uint32_t>({0});
    auto data2 = makeNumData<uint32_t>({10});
    auto r     = findNumChanged(makeMRV(std::move(data1)),
                                makeMRV(std::move(data2)), uint32_t(5));
    ASSERT_EQ(r.size(), 1);
    EXPECT_EQ(r[0].rAddr, 0);
    EXPECT_EQ(r[0].rSize, sizeof(uint32_t));
}

TEST(findNumChanged, MultipleValues)
{
    // values: 0->3 (no), 0->10 (yes), 0->2 (no), 0->7 (yes)
    auto data1 = makeNumData<uint32_t>({0, 0, 0, 0});
    auto data2 = makeNumData<uint32_t>({3, 10, 2, 7});
    auto r     = findNumChanged(makeMRV(std::move(data1)),
                                makeMRV(std::move(data2)), uint32_t(5));
    ASSERT_EQ(r.size(), 2);
    EXPECT_EQ(r[0].rAddr, 1 * sizeof(uint32_t));
    EXPECT_EQ(r[1].rAddr, 3 * sizeof(uint32_t));
}

TEST(findNumChanged, Prealignment)
{
    // pAddr=0x1001, rAddr=1 => tbegin()=0x1002, alignment=4
    // prealign = 4 + (0x1000) - 0x1002 = 2
    // bytesCompared starts at 2, straddling val1/val2 boundary
    // result.rAddr = map.rAddr(1) + bytesCompared(2) = 3
    auto data1 = makeNumData<uint32_t>({0, 0});
    auto data2 = makeNumData<uint32_t>({0, 15});
    auto r = findNumChanged(makeMRV(std::move(data1), 0x1001, 1),
                            makeMRV(std::move(data2), 0x1001, 1), uint32_t(5));
    ASSERT_EQ(r.size(), 1);
    EXPECT_EQ(r[0].rAddr, 3);
}

TEST(findNumChanged, SignedNegative)
{
    // diff = 5 - 10 = -5, minChange = 1, so -5 >= 1 is false
    auto data1 = makeNumData<int32_t>({10});
    auto data2 = makeNumData<int32_t>({5});
    auto r     = findNumChanged(makeMRV(std::move(data1)),
                                makeMRV(std::move(data2)), int32_t(1));
    EXPECT_TRUE(r.empty());
}

// -------------------------------------------------------------------
// findNumUnchanged
// -------------------------------------------------------------------

TEST(findNumUnchanged, NoChange)
{
    auto data1 = makeNumData<uint32_t>({5});
    auto data2 = makeNumData<uint32_t>({5});
    auto r     = findNumUnchanged(makeMRV(std::move(data1)),
                                  makeMRV(std::move(data2)), uint32_t(0));
    ASSERT_EQ(r.size(), 1);
    EXPECT_EQ(r[0].rAddr, 0);
    EXPECT_EQ(r[0].rSize, sizeof(uint32_t));
}

TEST(findNumUnchanged, AllChangedTooMuch)
{
    auto data1 = makeNumData<uint32_t>({0});
    auto data2 = makeNumData<uint32_t>({10});
    auto r     = findNumUnchanged(makeMRV(std::move(data1)),
                                  makeMRV(std::move(data2)), uint32_t(5));
    EXPECT_TRUE(r.empty());
}

TEST(findNumUnchanged, Boundary)
{
    auto data1 = makeNumData<uint32_t>({0});
    auto data2 = makeNumData<uint32_t>({5});
    auto r     = findNumUnchanged(makeMRV(std::move(data1)),
                                  makeMRV(std::move(data2)), uint32_t(5));
    ASSERT_EQ(r.size(), 1);
}

TEST(findNumUnchanged, BelowBoundary)
{
    auto data1 = makeNumData<uint32_t>({0});
    auto data2 = makeNumData<uint32_t>({3});
    auto r     = findNumUnchanged(makeMRV(std::move(data1)),
                                  makeMRV(std::move(data2)), uint32_t(5));
    ASSERT_EQ(r.size(), 1);
}

TEST(findNumUnchanged, Mixed)
{
    // values: 0->2 (yes, diff=2), 0->10 (no, diff=10), 0->4 (yes, diff=4)
    auto data1 = makeNumData<uint32_t>({0, 0, 0});
    auto data2 = makeNumData<uint32_t>({2, 10, 4});
    auto r     = findNumUnchanged(makeMRV(std::move(data1)),
                                  makeMRV(std::move(data2)), uint32_t(5));
    ASSERT_EQ(r.size(), 2);
    EXPECT_EQ(r[0].rAddr, 0);
    EXPECT_EQ(r[1].rAddr, 2 * sizeof(uint32_t));
}

// -------------------------------------------------------------------
// findString
// -------------------------------------------------------------------

TEST(findString, EmptyData)
{
    auto a = makeMRV({});
    auto r = findString(a, "hello");
    EXPECT_TRUE(r.empty());
}

TEST(findString, NoMatch)
{
    auto a = makeMRV({1, 2, 3, 4});
    auto r = findString(a, "hello");
    EXPECT_TRUE(r.empty());
}

TEST(findString, SingleMatch)
{
    std::vector<uint8_t> data = {'a', 'b', 'c', 'x', 'y', 'z'};
    auto                 r    = findString(makeMRV(std::move(data)), "abc");
    ASSERT_EQ(r.size(), 1);
    EXPECT_EQ(r[0].rAddr, 0);
    EXPECT_EQ(r[0].rSize, 3);
}

TEST(findString, MultipleMatches)
{
    std::vector<uint8_t> data = {'a', 'b', 'X', 'a', 'b', 'Y', 'a', 'b'};
    auto                 r    = findString(makeMRV(std::move(data)), "ab");
    ASSERT_EQ(r.size(), 3);
    EXPECT_EQ(r[0].rAddr, 0);
    EXPECT_EQ(r[1].rAddr, 3);
    EXPECT_EQ(r[2].rAddr, 6);
}

TEST(findString, MatchAtStart)
{
    std::vector<uint8_t> data = {'h', 'i', 'x', 'y'};
    auto                 r    = findString(makeMRV(std::move(data)), "hi");
    ASSERT_EQ(r.size(), 1);
    EXPECT_EQ(r[0].rAddr, 0);
}

TEST(findString, MatchAtEnd)
{
    std::vector<uint8_t> data = {'x', 'y', 'h', 'i'};
    auto                 r    = findString(makeMRV(std::move(data)), "hi");
    ASSERT_EQ(r.size(), 1);
    EXPECT_EQ(r[0].rAddr, 2);
}

TEST(findString, Overlapping)
{
    std::vector<uint8_t> data = {'a', 'a', 'a'};
    auto                 r    = findString(makeMRV(std::move(data)), "aa");
    ASSERT_EQ(r.size(), 2);
    EXPECT_EQ(r[0].rAddr, 0);
    EXPECT_EQ(r[1].rAddr, 1);
}

// -------------------------------------------------------------------
// findNumExact
// -------------------------------------------------------------------

TEST(findNumExact, NotFound)
{
    auto data = makeNumData<uint32_t>({1, 2, 3});
    auto r    = findNumExact(makeMRV(std::move(data)), uint32_t(99));
    EXPECT_TRUE(r.empty());
}

TEST(findNumExact, SingleMatch)
{
    auto data = makeNumData<uint32_t>({1, 2, 3, 4});
    auto r    = findNumExact(makeMRV(std::move(data)), uint32_t(3));
    ASSERT_EQ(r.size(), 1);
    EXPECT_EQ(r[0].rAddr, 2 * sizeof(uint32_t));
    EXPECT_EQ(r[0].rSize, sizeof(uint32_t));
}

TEST(findNumExact, MultipleMatches)
{
    auto data = makeNumData<uint32_t>({7, 1, 7, 2, 7});
    auto r    = findNumExact(makeMRV(std::move(data)), uint32_t(7));
    ASSERT_EQ(r.size(), 3);
    EXPECT_EQ(r[0].rAddr, 0);
    EXPECT_EQ(r[1].rAddr, 2 * sizeof(uint32_t));
    EXPECT_EQ(r[2].rAddr, 4 * sizeof(uint32_t));
}

TEST(findNumExact, Prealignment)
{
    // pAddr=0x1001, rAddr=1 => tbegin()=0x1002, prealign=2
    // value at straddle is 0x000F0000 = 983040, not 7
    // so only the actual 7 at offset 0 from data's perspective is tested
    auto data = makeNumData<uint32_t>({7, 0});
    auto r    = findNumExact(makeMRV(std::move(data), 0x1001, 1), uint32_t(7));
    EXPECT_TRUE(r.empty());
}

TEST(findNumExact, SignedNegative)
{
    auto data = makeNumData<int32_t>({10, -5, 3});
    auto r    = findNumExact(makeMRV(std::move(data)), int32_t(-5));
    ASSERT_EQ(r.size(), 1);
    EXPECT_EQ(r[0].rAddr, 1 * sizeof(int32_t));
}

// -------------------------------------------------------------------
// findNumInRange
// -------------------------------------------------------------------

TEST(findNumInRange, NoneInRange)
{
    auto data = makeNumData<uint32_t>({1, 2, 3});
    auto r =
        findNumInRange(makeMRV(std::move(data)), uint32_t(10), uint32_t(20));
    EXPECT_TRUE(r.empty());
}

TEST(findNumInRange, SingleInRange)
{
    auto data = makeNumData<uint32_t>({1, 5, 10, 20});
    auto r = findNumInRange(makeMRV(std::move(data)), uint32_t(4), uint32_t(6));
    ASSERT_EQ(r.size(), 1);
    EXPECT_EQ(r[0].rAddr, 1 * sizeof(uint32_t));
}

TEST(findNumInRange, AllInRange)
{
    auto data = makeNumData<uint32_t>({2, 3, 4});
    auto r = findNumInRange(makeMRV(std::move(data)), uint32_t(1), uint32_t(5));
    ASSERT_EQ(r.size(), 3);
}

TEST(findNumInRange, BoundaryValues)
{
    auto data = makeNumData<uint32_t>({5, 7, 10});
    auto r =
        findNumInRange(makeMRV(std::move(data)), uint32_t(5), uint32_t(10));
    ASSERT_EQ(r.size(), 3);
    EXPECT_EQ(r[0].rAddr, 0);
    EXPECT_EQ(r[1].rAddr, 1 * sizeof(uint32_t));
    EXPECT_EQ(r[2].rAddr, 2 * sizeof(uint32_t));
}

TEST(findNumInRange, Prealignment)
{
    auto data = makeNumData<uint32_t>({5, 10});
    auto r    = findNumInRange(makeMRV(std::move(data), 0x1001, 1), uint32_t(5),
                               uint32_t(10));
    EXPECT_TRUE(r.empty());
}

TEST(findNumInRange, SignedRange)
{
    auto data = makeNumData<int32_t>({-10, -2, 0, 3, 10});
    auto r = findNumInRange(makeMRV(std::move(data)), int32_t(-5), int32_t(5));
    ASSERT_EQ(r.size(), 3);
    EXPECT_EQ(r[0].rAddr, 1 * sizeof(int32_t));
    EXPECT_EQ(r[1].rAddr, 2 * sizeof(int32_t));
    EXPECT_EQ(r[2].rAddr, 3 * sizeof(int32_t));
}

// -------------------------------------------------------------------
// findUnchanged
// -------------------------------------------------------------------

TEST(findUnchanged, Empty)
{
    auto a = makeMRV({});
    auto b = makeMRV({});
    auto r = findUnchanged(a, b, 1);
    EXPECT_TRUE(r.empty());
}

TEST(findUnchanged, AllIdentical)
{
    auto a = makeMRV({1, 2, 3, 4});
    auto b = makeMRV({1, 2, 3, 4});
    auto r = findUnchanged(a, b, 1);
    ASSERT_EQ(r.size(), 1);
    EXPECT_EQ(r[0].rAddr, 0);
    EXPECT_EQ(r[0].rSize, 4);
}

TEST(findUnchanged, AllDifferent)
{
    auto a = makeMRV({0, 0, 0, 0});
    auto b = makeMRV({1, 1, 1, 1});
    auto r = findUnchanged(a, b, 1);
    EXPECT_TRUE(r.empty());
}

TEST(findUnchanged, PartialUnchanged)
{
    std::vector<uint8_t> a(12, 0);
    std::vector<uint8_t> b(12, 0);
    for (int i = 0; i < 12; ++i)
        b[i] = 1;
    for (int i = 4; i < 8; ++i)
        b[i] = 0;

    auto r = findUnchanged(makeMRV(std::move(a)), makeMRV(std::move(b)), 1);
    ASSERT_EQ(r.size(), 1);
    EXPECT_EQ(r[0].rAddr, 4);
    EXPECT_EQ(r[0].rSize, 4);
}

TEST(findUnchanged, AdjacentMerge)
{
    std::vector<uint8_t> a(8, 0);
    std::vector<uint8_t> b(8, 0);
    for (int i = 0; i < 8; ++i)
        b[i] = 100;
    for (int i = 0; i < 4; ++i)
        b[i] = 0;

    auto r = findUnchanged(makeMRV(std::move(a)), makeMRV(std::move(b)), 2);
    ASSERT_EQ(r.size(), 1);
    EXPECT_EQ(r[0].rAddr, 0);
    EXPECT_EQ(r[0].rSize, 4);
}
