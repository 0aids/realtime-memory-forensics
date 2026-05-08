#include <gtest/gtest.h>
#include <print>
#include <rmf/logging/logging.hpp>
#include <rmf/rmf.hpp>
#include <rmf/utils/expect.hpp>
#include <cstring>

namespace mf  = RealtimeMemoryForensics;
namespace mfl = mf::Logging;
namespace mfu = mf::Utils;

TEST(Expected, unexpectedErr)
{
    using namespace std;
    auto err = rmf_mkErr(mfu::ErrorEnum::TestError);
    println("{}", err.what());
    println("Updating error");
    rmf_updErr(err, mfu::ErrorEnum::TestError);
    println("{}", err.what());
}

TEST(Expected, functionReturnsError)
{
    using namespace std;
    auto errGen = []() -> mfu::ErrU<size_t>
    {
        auto err = rmf_mkErr(mfu::ErrorEnum::TestError);
        return err;
    };
    auto errGen2 = [errGen]() -> mfu::ErrU<size_t>
    {
        if (auto err = errGen(); !err.has_value())
        {
            rmf_updRetErr(err, mfu::ErrorEnum::TestError);
        }
        return 10;
    };
    println("{}", (errGen2()).error().what());
}

TEST(Expected, Error_messageContainsLocation)
{
    auto        err = rmf_mkErr(mfu::ErrorEnum::TestError);
    const char* msg = err.what();

    std::println("what: {}", msg);
    EXPECT_TRUE(std::strstr(msg, "expect.cpp") != nullptr);
}

TEST(Expected, rmf_mkErr_createsError)
{
    auto           err    = rmf_mkErr(mfu::ErrorEnum::TestError);
    mfu::ErrU<int> result = err;

    EXPECT_FALSE(result.has_value());
    EXPECT_NO_THROW([[maybe_unused]] auto e = result.error());
}

TEST(Expected, rmf_updErr_updatesError)
{
    auto        err       = rmf_mkErr(mfu::ErrorEnum::TestError);
    std::string msgBefore = err.what();

    rmf_updErr(err, mfu::ErrorEnum::TestError);
    std::string msgAfter = err.what();

    EXPECT_NE(msgBefore, msgAfter);
}

TEST(Expected, rmf_retErr_propagatesError)
{
    auto createErr = []() -> mfu::ErrU<int>
    { return rmf_mkErr(mfu::ErrorEnum::TestError); };

    auto passThrough = [&createErr]() -> mfu::ErrU<int>
    {
        rmf_retErr(createErr());
        return 42;
    };

    auto result = passThrough();
    EXPECT_NO_THROW(result.error());
    EXPECT_FALSE(result.has_value());
}

TEST(Expected, rmf_updRetErr_updatesAndReturns)
{
    auto createErr = []() -> mfu::ErrU<int>
    { return rmf_mkErr(mfu::ErrorEnum::TestError); };

    auto passThrough = [&createErr]() -> mfu::ErrU<int>
    {
        mfu::ErrU<int> errU;
        rmf_updRetErr(createErr(), mfu::ErrorEnum::MaxErrorDepthReached);
        return 42;
    };

    auto result = passThrough();
    EXPECT_FALSE(result.has_value());
}
