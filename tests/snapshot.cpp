#include <gtest/gtest.h>
#include <print>
#include <rmf/logging/logging.hpp>
#include <rmf/rmf.hpp>
#include <rmf/utils/expect.hpp>
#include <rmf/utils/str.hpp>
#include <rmf/node.hpp>
#include <rmf/map.hpp>
#include <rmf/snapshot.hpp>
#include "helpers.hpp"
#include "rmf/test_helpers.hpp"
#include "rmf/utils/function.hpp"
#include "rmf/op.hpp"
#include "rmf/utils/threadpool.hpp"

using namespace std;
namespace mf  = RealtimeMemoryForensics;
namespace mfl = mf::Logging;
namespace mfu = mf::Utils;
namespace mft = mf::Tests;

template <typename T>
concept requiresMap = requires(T t) { t.wellFormed(); };

TEST(snapshot, static_assertions)
{
    mf::Node<mf::Snapshot> gah;
    static_assert(requiresMap<mf::Node<mf::Snapshot>>,
                  "Should require an mf::map!");
    static_assert(
        requires { mf ::Node<mf ::Map, mf ::Snapshot>{}; },
        "Should require an mf::map!");
}

TEST(snapshot, fakeBuffer)
{
    using namespace mf;
    std::vector<uint8_t> fakeBuffer(0xffff, 0xff);
    Node<Snapshot, Map>  value =
        Node<Snapshot, Map>::fromBuffer(std::move(fakeBuffer));
    EXPECT_NO_THROW(value.wellFormed());
    EXPECT_EQ(value.span()[0], 0xff);
}

TEST(snapshot, findString)
{
    const char       str[]   = "Hello world!";
    constexpr size_t bufSize = 1024;
    size_t           head    = 0;
    auto             buffer  = mft::TestBuffer::makeZeroed(bufSize);
    ssize_t          diff1   = 100;
    EXPECT_EQ(static_cast<bool>(buffer.pushPadding(diff1)),
              (head += diff1) < bufSize);
    // println("Expected - diff: {}, head: {}, actualOffset: {}", diff1,
    //         head, buffer.headOffset());
    ssize_t diff2 = 1024;
    EXPECT_EQ(static_cast<bool>(buffer.pushPadding(diff2)),
              head + diff2 < bufSize);
    // println("Expected - diff: {}, head: {}, actualOffset: {}", diff2,
    //         head, buffer.headOffset());
    ssize_t diff3 = sizeof(str);
    EXPECT_EQ(static_cast<bool>(buffer.push(str)),
              (head += diff3) < bufSize);
    ssize_t diff4 = bufSize - head - 3;
    // println("Expected - diff: {}, head: {}, actualOffset: {}", diff3,
    //         head, buffer.headOffset());
    EXPECT_EQ(static_cast<bool>(buffer.pushPadding(diff4)),
              (head += diff4) < bufSize);
    ssize_t diff5 = sizeof(str);
    // println("Expected - diff: {}, head: {}, actualOffset: {}", diff4,
    //         head, buffer.headOffset());
    EXPECT_EQ(static_cast<bool>(buffer.push(str)),
              (head += diff5) < bufSize);
    // println("Expected - diff: {}, head: {}, actualOffset: {}", diff5,
    //         head, buffer.headOffset());
    EXPECT_LE(buffer.chead(), buffer.cend());
    EXPECT_LE(buffer.chead(), buffer.cend());
    // println("head: {}", buffer.chead() - buffer.cbegin());
}
TEST(snapshot, findNumExact)
{
    using namespace mf;
    // TODO: Cleanup this and the above test.
    uint64_t         num     = 0x1234567890abcdef;
    constexpr size_t bufSize = 1024;
    auto             buffer  = mft::TestBuffer::makeZeroed(bufSize);
    ssize_t          diff1   = 100;
    EXPECT_TRUE(static_cast<bool>(buffer.pushPadding(diff1)));
    ssize_t diff2 = 1024;
    EXPECT_FALSE(static_cast<bool>(buffer.pushPadding(diff2)));
    EXPECT_TRUE(static_cast<bool>(buffer.push(num)));
    EXPECT_TRUE(static_cast<bool>(buffer.pushPadding(diff1)));
    EXPECT_TRUE(static_cast<bool>(buffer.push(num)));
    EXPECT_LE(buffer.chead(), buffer.cend());
    EXPECT_LE(buffer.chead(), buffer.cend());
    auto snapshot =
        Node<Snapshot, Map>::fromBuffer(buffer.moveBuffer());
    // copy
    auto snapshot1 = snapshot;
    // Figure out how to shorten this.
    // Holy shit it's so difficult.
    auto res = findNumExact(snapshot, num);
    EXPECT_EQ(res.size(), 2);
    Utils::ThreadPool tp(1);

    auto              snaps = mfu::Vec<decltype(snapshot)>{snapshot};
    println("inputs: {}", snaps.size());
    auto res1 = findNumExact.threaded(snaps, num).with(tp);
    println("res1: {}", res1[0].size());
    EXPECT_EQ(res1[0].size(), res.size());
}
