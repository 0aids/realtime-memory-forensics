#include "rmf/utils/threadpool.hpp"
#include <gtest/gtest.h>
#include <rmf/logging/logging.hpp>
#include <rmf/rmf.hpp>
#include <rmf/utils/function.hpp>
#include <print>

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
    auto newFunc = mfu::Function(testFuncReturnsInt);
    println("Result: {}", newFunc());
}

TEST(function, lambda)
{
    auto lambda  = []() { return 10; };
    auto newFunc = mfu::Function(lambda);
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
    auto newFunc = mfu::Function(doubleNum);
    EXPECT_EQ(newFunc(10), doubleNum(10));
}

TEST(function, perfectForwarding)
{
    static size_t _i = 0;
    auto          a  = [](StructorTest&& t) { return t.numCopies; };

    auto          newFunc = mfu::Function(a);
    newFunc(StructorTest{});
    EXPECT_EQ(_i, 0);
}

TEST(function, Function_noCopySemantics)
{
    auto func = mfu::Function(+[]() { return 42; });
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
    auto func =
        mfu::Function(+[](MoveOnlyType m) { return m.value; });
    MoveOnlyType mobj(42);
    EXPECT_EQ(func(std::move(mobj)), 42);
    EXPECT_EQ(mobj.value, 0);
}

TEST(function, Function_returnTypeCorrect)
{
    auto intFunc = mfu::Function(+[]() { return 42; });
    auto strFunc =
        mfu::Function(+[]() { return std::string("hello"); });
    auto doubleFunc = mfu::Function(+[]() { return 3.14; });

    EXPECT_EQ(intFunc(), 42);
    EXPECT_EQ(strFunc(), std::string("hello"));
    EXPECT_DOUBLE_EQ(doubleFunc(), 3.14);
}

TEST(function, Function_variousArgTypes)
{
    auto funcInt = mfu::Function(+[](int x) { return x * 2; });
    auto funcStr =
        mfu::Function(+[](const std::string& s) { return s.size(); });
    auto funcFloat = mfu::Function(+[](double d) { return d + 1.0; });

    EXPECT_EQ(funcInt(5), 10);
    EXPECT_EQ(funcStr(std::string("hello")), 5);
    EXPECT_DOUBLE_EQ(funcFloat(2.5), 3.5);
}

TEST(function, Function_threader)
{
    auto funcInt = mfu::Function([](int x) { return x * 2; });
    mfu::ThreadPool tp(1);
    vector<int>     a = {1, 2, 3, 4, 5};
    funcInt.threaded(a).with(tp);
    funcInt.threaded(a).with(tp);
}

size_t moveTests(StructorTest&& T)
{ return T.numCopies; }

TEST(function, Function_structors)
{
    auto                 funcInt = mfu::Function(moveTests);
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
    auto            funcInt = mfu::Function(copy);
    mfu::ThreadPool tp(1);
    vector<StructorTest> start;
    for (size_t i = 0; i < 100; i++)
        start.emplace_back();

    println("Forwarding from here...");
    auto r = funcInt.threaded(start).with(tp);
    println("Num copies called: {}", r.front());
    EXPECT_LE(r.front(), 0);
}
