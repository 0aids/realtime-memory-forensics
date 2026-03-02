#include <gtest/gtest.h>
#include <memory>
#include "test_helpers.hpp"


TEST(componentTest, incrementingIntComponentBasic)
{
    using namespace rmf::test;

    rmf::test::testProcess tp;
    tp.build<incrementingIntComponent>(10, 5);
    pid_t pid = tp.run();

    ASSERT_GT(pid, 0);

    std::this_thread::sleep_for(std::chrono::seconds(2));

    tp.stop();
}

TEST(componentTest, staticValueComponentBasic)
{
    using namespace rmf::test;

    rmf::test::testProcess tp;
    tp.build<staticValueComponent>();
    pid_t pid = tp.run();

    ASSERT_GT(pid, 0);

    std::this_thread::sleep_for(std::chrono::seconds(1));

    tp.stop();
}

TEST(componentTest, staticStringComponentBasic)
{
    using namespace rmf::test;

    rmf::test::testProcess tp;
    tp.build<staticStringTestComponent>();
    pid_t pid = tp.run();

    ASSERT_GT(pid, 0);

    std::this_thread::sleep_for(std::chrono::seconds(1));

    tp.stop();
}

TEST(componentTest, multipleComponents)
{
    using namespace rmf::test;

    rmf::test::testProcess tp;
    tp.build<incrementingIntComponent>(0, 1)
        .build<staticValueComponent>()
        .build<staticStringTestComponent>();
    pid_t pid = tp.run();

    ASSERT_GT(pid, 0);

    std::this_thread::sleep_for(std::chrono::seconds(1));

    tp.stop();
}
