#include <gtest/gtest.h>
#include <print>
#include <rmf/logging/logging.hpp>
#include <rmf/rmf.hpp>
#include <rmf/utils/expect.hpp>

namespace mf  = RealtimeMemoryForensics;
namespace mfl = mf::Logging;
namespace mfu = mf::Utils;
// Testing expected
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
