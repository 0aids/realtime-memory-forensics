#include <gtest/gtest.h>
#include <vector>
#include <thread>
#include <atomic>
#include "rmf.hpp"

using namespace rmf;

TEST(AnalyzerTest, SingleArgExecution)
{
    Analyzer analyzer(2);

    auto results = analyzer.Execute([](int x) { return x * 2; }, 5);

    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0], 10);
}

TEST(AnalyzerTest, ContainerExecution)
{
    Analyzer         analyzer(4);
    std::vector<int> input = {1, 2, 3, 4, 5};

    auto             results =
        analyzer.Execute([](int x) { return x * 2; }, input);

    ASSERT_EQ(results.size(), 5);
    EXPECT_EQ(results[0], 2);
    EXPECT_EQ(results[1], 4);
    EXPECT_EQ(results[2], 6);
    EXPECT_EQ(results[3], 8);
    EXPECT_EQ(results[4], 10);
}

TEST(AnalyzerTest, ReturnValuesCorrect)
{
    Analyzer         analyzer(4);
    std::vector<int> input = {10, 20, 30};

    auto             results =
        analyzer.Execute([](int x) { return x + x; }, input);

    ASSERT_EQ(results.size(), 3);
    EXPECT_EQ(results[0], 20);
    EXPECT_EQ(results[1], 40);
    EXPECT_EQ(results[2], 60);
}

TEST(AnalyzerTest, MultipleExecuteCalls)
{
    Analyzer analyzer(2);

    auto results1 = analyzer.Execute([](int x) { return x * 2; }, 5);
    auto results2 = analyzer.Execute([](int x) { return x + 1; }, 10);

    ASSERT_EQ(results1.size(), 1);
    ASSERT_EQ(results2.size(), 1);
    EXPECT_EQ(results1[0], 10);
    EXPECT_EQ(results2[0], 11);
}

TEST(AnalyzerTest, ParallelExecution)
{
    std::atomic<int> maxConcurrent{0};
    std::atomic<int> currentConcurrent{0};

    auto             incrementingFunc = [&](int)
    {
        int val     = currentConcurrent.fetch_add(1) + 1;
        int prevMax = maxConcurrent.load();
        while (val > prevMax &&
               !maxConcurrent.compare_exchange_weak(prevMax, val))
        {
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        currentConcurrent.fetch_sub(1);
        return val;
    };

    Analyzer         analyzer(4);
    std::vector<int> input(10, 0);

    auto results = analyzer.Execute(incrementingFunc, input);

    EXPECT_GE(maxConcurrent.load(), 1);
    EXPECT_EQ(results.size(), 10);
}

TEST(AnalyzerTest, DifferentReturnTypes)
{
    Analyzer analyzer(2);

    auto     intResults =
        analyzer.Execute([](int x) { return x * 2; }, 5);
    ASSERT_EQ(intResults.size(), 1);
    EXPECT_EQ(intResults[0], 10);

    auto doubleResults =
        analyzer.Execute([](double x) { return x * 1.5; }, 4.0);
    ASSERT_EQ(doubleResults.size(), 1);
    EXPECT_DOUBLE_EQ(doubleResults[0], 6.0);
}
