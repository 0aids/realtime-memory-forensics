#include <gtest/gtest.h>
#include <rmf/logging/logging.hpp>
#include <rmf/rmf.hpp>
#include <rmf/utils/function.hpp>
#include <print>

using namespace std;
namespace mf  = RealtimeMemoryForensics;
namespace mfu = mf::Utils;
namespace mfl = mf::Logging;

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
    struct test
    {
        int t  = 0;
        test() = default;
        test(const test&)
        {
            _i++;
            println("copy constructor called");
        }
        test(test&&)
        {
            _i++;
            println("move constructor called");
        };
        test& operator=(const test&)
        {
            _i++;
            println("copy assignment called");
            return *this;
        }
        test& operator=(test&&)
        {
            println("move assignment called");
            _i++;
            return *this;
        }
    };
    auto a = [](test&& t) { return t.t; };

    auto newFunc = mfu::Function(a);
    newFunc(test{});
    EXPECT_EQ(_i, 0);
}
