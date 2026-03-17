#include <gtest/gtest.h>
#include <memory>
#include "test_helpers.hpp"

TEST(testProcessTest, BasicAssertions)
{
    using namespace rmf::test;
    EXPECT_EQ(1 + 2, 3);

    testProcess tp;
    tp.build<testComponent>().build<staticStringTestComponent>();
    EXPECT_GE(tp.run(), 0);
}
