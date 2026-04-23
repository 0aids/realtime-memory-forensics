#include "rmf/utils/threadpool.hpp"
#include <gtest/gtest.h>
#include <rmf/logging/logging.hpp>
#include <rmf/rmf.hpp>
#include <rmf/utils/function.hpp>
#include <print>
#include "helpers.hpp"

using namespace std;
namespace mf  = RealtimeMemoryForensics;
namespace mfu = mf::Utils;
namespace mfl = mf::Logging;

using namespace std;
struct StructorTest
{
    int numCopies  = 0;
    int numMoves   = 0;
    StructorTest() = default;
    StructorTest(const StructorTest&)
    {
        numCopies++;
        println("copy constructor called");
    }
    StructorTest(StructorTest&&)
    {
        numMoves++;
        println("move constructor called");
    };
    StructorTest& operator=(const StructorTest&)
    {
        numCopies++;
        println("copy assignment called");
        return *this;
    }
    StructorTest& operator=(StructorTest&&)
    {
        numMoves++;
        println("move assignment called");
        return *this;
    }
};
size_t testFuncReturnsInt()
{
    static size_t i = 0;
    return i++;
}

TEST(function, testFunction)
{
    constexpr auto newFunc = mfu::Function(testFuncReturnsInt);
    println("Result: {}", newFunc());
}

TEST(function, lambda)
{
    auto           lambda  = []() { return 10; };
    constexpr auto newFunc = mfu::Function(lambda);
    println("Result: {}", newFunc());
}

TEST(function, stdfunction)
{
    auto lambda  = std::function([]() { return 10; });
    auto newFunc = mfu::Function(lambda);
    println("Result: {}", newFunc());
}

size_t doubleNum(size_t num)
{ return num * 2; }

TEST(function, withInputs)
{
    constexpr auto newFunc = mfu::Function(doubleNum);
    EXPECT_EQ(newFunc(10), doubleNum(10));
}

TEST(function, perfectForwarding)
{
    static size_t  _i = 0;
    auto           a  = [](StructorTest&& t) { return t.numCopies; };

    constexpr auto newFunc = mfu::Function(a);
    newFunc(StructorTest{});
    EXPECT_EQ(_i, 0);
}

TEST(function, Function_noCopySemantics)
{
    constexpr auto func = mfu::Function(+[]() { return 42; });
    static_assert(!std::is_copy_constructible_v<decltype(func)>,
                  "Function should not be copy constructible");
    static_assert(!std::is_copy_assignable_v<decltype(func)>,
                  "Function should not be copy assignable");
}

struct MoveOnlyType
{
    int value;
    MoveOnlyType(int v) : value(v) {}
    MoveOnlyType(const MoveOnlyType&)            = delete;
    MoveOnlyType& operator=(const MoveOnlyType&) = delete;
    MoveOnlyType(MoveOnlyType&& other) : value(other.value)
    { other.value = 0; }
    MoveOnlyType& operator=(MoveOnlyType&& other)
    {
        value       = other.value;
        other.value = 0;
        return *this;
    }
};

TEST(function, Function_moveOnlyCallable)
{
    constexpr auto func =
        mfu::Function(+[](MoveOnlyType m) { return m.value; });
    MoveOnlyType mobj(42);
    EXPECT_EQ(func(std::move(mobj)), 42);
    EXPECT_EQ(mobj.value, 0);
}

TEST(function, Function_returnTypeCorrect)
{
    constexpr auto intFunc = mfu::Function(+[]() { return 42; });
    constexpr auto strFunc =
        mfu::Function(+[]() { return std::string("hello"); });
    constexpr auto doubleFunc = mfu::Function(+[]() { return 3.14; });

    EXPECT_EQ(intFunc(), 42);
    EXPECT_EQ(strFunc(), std::string("hello"));
    EXPECT_DOUBLE_EQ(doubleFunc(), 3.14);
}

TEST(function, Function_variousArgTypes)
{
    constexpr auto funcInt =
        mfu::Function(+[](int x) { return x * 2; });
    constexpr auto funcStr =
        mfu::Function(+[](const std::string& s) { return s.size(); });
    constexpr auto funcFloat =
        mfu::Function(+[](double d) { return d + 1.0; });

    EXPECT_EQ(funcInt(5), 10);
    EXPECT_EQ(funcStr(std::string("hello")), 5);
    EXPECT_DOUBLE_EQ(funcFloat(2.5), 3.5);
}

TEST(function, Function_threader)
{
    constexpr auto funcInt =
        mfu::Function([](int x) { return x * 2; });
    mfu::ThreadPool tp(1);
    vector<int>     a = {1, 2, 3, 4, 5};
    funcInt.threaded(a).with(tp);
    funcInt.threaded(a).with(tp);
}

size_t moveTests(StructorTest&& T)
{ return T.numCopies; }

TEST(function, Function_structors)
{
    constexpr auto       funcInt = mfu::Function(moveTests);
    mfu::ThreadPool      tp(1);
    vector<StructorTest> start;
    start.emplace_back();
    println("Forwarding from here...");
    auto r = funcInt.threaded(start).with(tp);
    println("Num copies called: {}", r.front());
    EXPECT_LE(r.front(), 0);
}

TEST(function, Function_singleCopy)
{
    auto            copy = [](StructorTest T) { return T.numCopies; };
    constexpr auto  funcInt = mfu::Function(copy);
    mfu::ThreadPool tp(1);
    vector<StructorTest> start;
    for (size_t i = 0; i < 100; i++)
        start.emplace_back();

    println("Forwarding from here...");
    auto r = funcInt.threaded(start).with(tp);
    println("Num copies called: {}", r.front());
    EXPECT_LE(r.front(), 1);
}

TEST(function, threaded_returnsCorrectValues)
{
    constexpr auto funcInt =
        mfu::Function([](int x) { return x * 2; });
    mfu::ThreadPool tp(1);
    vector<int>     a        = {1, 2, 3, 4, 5};
    auto            r        = funcInt.threaded(a).with(tp);
    vector<int>     expected = {2, 4, 6, 8, 10};
    ASSERT_EQ(r.size(), expected.size());
    for (size_t i = 0; i < expected.size(); i++)
    {
        EXPECT_EQ(r[i], expected[i]);
    }
}

TEST(function, threaded_emptyVector)
{
    constexpr auto funcInt =
        mfu::Function([](int x) { return x * 2; });
    mfu::ThreadPool tp(1);
    vector<int>     a = {};
    auto            r = funcInt.threaded(a).with(tp);
    EXPECT_TRUE(r.empty());
}

TEST(function, threaded_singleElement)
{
    constexpr auto funcInt =
        mfu::Function([](int x) { return x * 2; });
    mfu::ThreadPool tp(1);
    vector<int>     a = {42};
    auto            r = funcInt.threaded(a).with(tp);
    ASSERT_EQ(r.size(), 1);
    EXPECT_EQ(r[0], 84);
}

TEST(function, threaded_mixedVectorAndScalar)
{
    constexpr auto func =
        mfu::Function([](int x, const string& prefix)
                      { return prefix + to_string(x); });
    mfu::ThreadPool tp(1);
    vector<int>     nums = {1, 2, 3};
    auto            r = func.threaded(nums, string("num_")).with(tp);
    ASSERT_EQ(r.size(), 3);
    EXPECT_EQ(r[0], "num_1");
    EXPECT_EQ(r[1], "num_2");
    EXPECT_EQ(r[2], "num_3");
}

TEST(function, threaded_multipleVectors_sameLength)
{
    constexpr auto func =
        mfu::Function([](int x, int y) { return x + y; });
    mfu::ThreadPool tp(1);
    vector<int>     a = {1, 2, 3};
    vector<int>     b = {10, 20, 30};
    auto            r = func.threaded(a, b).with(tp);
    ASSERT_EQ(r.size(), 3);
    EXPECT_EQ(r[0], 11);
    EXPECT_EQ(r[1], 22);
    EXPECT_EQ(r[2], 33);
}

struct MoveTracker
{
    int numCopies = 0;
    int numMoves  = 0;
    int value;
    MoveTracker() = default;
    MoveTracker(int v) : value(v) {}
    MoveTracker(const MoveTracker& other) : value(other.value)
    { numCopies++; }
    MoveTracker(MoveTracker&& other) noexcept : value(other.value)
    {
        other.value = 0;
        numMoves++;
    }
    MoveTracker& operator=(const MoveTracker& other)
    {
        value = other.value;
        numCopies++;
        return *this;
    }
    MoveTracker& operator=(MoveTracker&& other) noexcept
    {
        value       = other.value;
        other.value = 0;
        numMoves++;
        return *this;
    }
};

TEST(function, threaded_perfectForwarding_byValue)
{
    constexpr auto func =
        mfu::Function([](MoveTracker t) { return t.value; });
    mfu::ThreadPool     tp(1);
    vector<MoveTracker> items;
    for (int i = 0; i < 5; i++)
        items.emplace_back(i * 10);
    auto r = func.threaded(items).with(tp);
    ASSERT_EQ(r.size(), 5);
    EXPECT_EQ(r[0], 0);
    EXPECT_EQ(r[1], 10);
    EXPECT_EQ(r[2], 20);
    EXPECT_EQ(r[3], 30);
    EXPECT_EQ(r[4], 40);
    for (auto& item : items)
    {
        EXPECT_EQ(item.value, 0);
    }
}

TEST(function, threaded_perfectForwarding_lvalueRef)
{
    constexpr auto func =
        mfu::Function([](const MoveTracker& t) { return t.value; });
    mfu::ThreadPool     tp(1);
    vector<MoveTracker> items;
    for (int i = 0; i < 5; i++)
        items.emplace_back(i * 10);
    auto r = func.threaded(items).with(tp);
    ASSERT_EQ(r.size(), 5);
    EXPECT_EQ(r[0], 0);
    EXPECT_EQ(r[1], 10);
    for (size_t i = 0; i < items.size(); i++)
    {
        EXPECT_EQ(items[i].value, i * 10);
    }
}

TEST(function, threaded_unequalVectorLengths)
{
    constexpr auto func =
        mfu::Function([](int x, int y) { return x + y; });
    mfu::ThreadPool tp(1);
    vector<int>     a        = {1, 2, 3};
    vector<int>     b        = {10, 20};
    auto            threader = func.threaded(a, b);
    auto            r        = threader.with(tp);
    EXPECT_TRUE(r.empty());
}
template <typename T>
concept can_thread_empty = requires(T t) { t.template threaded<>(); };

/* This succeeds
 * static_assert(!can_thread_empty<decltype(func)>, 
                  "threaded() shouldn't work for no-arguments!");
 * But this fails
 * static_assert(!requires (decltype(func) f) {f.template threaded<>(),
                  "threaded() shouldn't work for no-arguments!");
*/
TEST(function, threaded_emptyThreader_with)
{
    constexpr auto  func = mfu::Function([](int x) { return x; });
    mfu::ThreadPool tp(1);
    // Direct, generic, and readable.
    // Checks if calling func.threaded() with no arguments is well-formed.
    static_assert(
        requires { func.template threaded<int>(1).with(tp); },
        "threaded() SHOULD work with an int and thread pool!");
    static_assert(!can_thread_empty<decltype(func)>,
                  "threaded() shouldn't work for no-arguments!");
}
