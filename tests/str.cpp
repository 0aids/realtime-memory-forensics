#include <gtest/gtest.h>
#include <rmf/logging/logging.hpp>
#include <rmf/rmf.hpp>
#include <rmf/utils/expect.hpp>
#include <rmf/utils/str.hpp>

namespace mf  = RealtimeMemoryForensics;
namespace mfl = mf::Logging;
namespace mfu = mf::Utils;
TEST(strFormatWrapper, formatBasic)
{
    using namespace mfu::Literals;
    std::string result = "abc def 123";
    EXPECT_EQ("{} {} {}"_f.fmt("abc", "def", 123), result);
}
