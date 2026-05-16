#include <gtest/gtest.h>
#include <rmf/logging/logging.hpp>
#include <rmf/rmf.hpp>
#include <rmf/utils/expect.hpp>
#include <rmf/utils/str.hpp>

namespace mf  = rmf;
namespace mfl = mf::Logging;
namespace mfu = mf::Utils;

TEST(strFormatWrapper, formatBasic)
{
    using namespace mfu::Literals;
    std::string result = "abc def 123";
    EXPECT_EQ("{} {} {}"_f.fmt("abc", "def", 123), result);
}

TEST(strFormatWrapper, formatSingleArg)
{
    using namespace mfu::Literals;
    EXPECT_EQ("value: {}"_f.fmt(42), "value: 42");
}

TEST(strFormatWrapper, formatIntArgs)
{
    using namespace mfu::Literals;
    EXPECT_EQ("{}+{}={}"_f.fmt(1, 2, 3), "1+2=3");
    EXPECT_EQ("{}*{}={}"_f.fmt(6, 7, 42), "6*7=42");
}

TEST(strFormatWrapper, formatStringArg)
{
    using namespace mfu::Literals;
    EXPECT_EQ("hello {}"_f.fmt("world"), "hello world");
    EXPECT_EQ("{} {}"_f.fmt("a", "b"), "a b");
}

TEST(strFormatWrapper, formatFloatArg)
{
    using namespace mfu::Literals;
    EXPECT_EQ("pi={}"_f.fmt(3.14), "pi=3.14");
    EXPECT_EQ("{} {}"_f.fmt(1.5, 2.5), "1.5 2.5");
}

TEST(strFormatWrapper, formatBoolArg)
{
    using namespace mfu::Literals;
    EXPECT_EQ("flag={}"_f.fmt(true), "flag=true");
    EXPECT_EQ("flag={}"_f.fmt(false), "flag=false");
}

TEST(strFormatWrapper, formatMixedTypes)
{
    using namespace mfu::Literals;
    std::string expected = "count 42, pi 3.14, ok true";
    EXPECT_EQ("count {}, pi {}, ok {}"_f.fmt(42, 3.14, true), expected);
}

TEST(strFormatWrapper, formatEmptyString)
{
    using namespace mfu::Literals;
    EXPECT_EQ(""_f.fmt(), "");
}

TEST(strFormatWrapper, formatNoArgs)
{
    using namespace mfu::Literals;
    EXPECT_EQ("no args"_f.fmt(), "no args");
}
