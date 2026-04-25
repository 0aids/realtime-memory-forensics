#include <gtest/gtest.h>
#include "rmf/test_helpers.hpp"

using namespace std;
namespace mf  = RealtimeMemoryForensics;
namespace mft = mf::Tests;

using namespace mft;
TEST(testProgram, staticStringNumber)
{
    forkFunc(createTestProgram(
        StaticNumberBuffer<int, 0xfafaf>(), TestFeature{},
        StaticStringBuffer{.buffer = "hello world"}));
}

TEST(testProgram, findStaticNumber)
{
    pid_t pid = forkFunc(createTestProgram(
        StaticNumberBuffer<int, 0xfafaf>(), TestFeature{},
        StaticStringBuffer{.buffer = "hello world"}));
    // get maps from pid!
}
