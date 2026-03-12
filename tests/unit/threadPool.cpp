#include <gtest/gtest.h>
#include <atomic>
#include <thread>
#include <vector>
#include <chrono>
#include "multi_threading.hpp"

using namespace rmf;

TEST(TaskThreadPoolTest, BasicTaskExecution)
{
    std::atomic<bool> executed{false};
    {
        TaskThreadPool_t pool(2);
        Task_t<void>     task([&]() { executed.store(true); });
        pool.SubmitTask(task);
        pool.AwaitTasks();
    }
    EXPECT_TRUE(executed.load());
}

TEST(TaskThreadPoolTest, TaskWithReturnValue)
{
    {
        TaskThreadPool_t pool(2);
        Task_t<int>      task([]() { return 42; });
        pool.SubmitTask(task);
        pool.AwaitTasks();
        EXPECT_EQ(task.getFuture().get(), 42);
    }
}

TEST(TaskThreadPoolTest, MultipleTaskExecution)
{
    std::atomic<int> counter{0};
    const int        taskCount = 100;
    {
        TaskThreadPool_t          pool(4);
        std::vector<Task_t<void>> tasks;
        tasks.reserve(taskCount);

        for (int i = 0; i < taskCount; i++)
        {
            tasks.emplace_back([&]() { counter.fetch_add(1); });
            pool.SubmitTask(tasks.back());
        }
        pool.AwaitTasks();
    }
    EXPECT_EQ(counter.load(), taskCount);
}

TEST(TaskThreadPoolTest, ThreadCount)
{
    const size_t        numThreads = 4;
    std::atomic<size_t> activeThreads{0};
    std::atomic<size_t> maxActive{0};

    {
        TaskThreadPool_t          pool(numThreads);
        std::vector<Task_t<void>> tasks;

        for (int i = 0; i < 10; i++)
        {
            tasks.emplace_back(
                [&]()
                {
                    size_t current = activeThreads.fetch_add(1) + 1;
                    size_t prevMax = maxActive.load();
                    while (current > prevMax &&
                           !maxActive.compare_exchange_weak(prevMax,
                                                            current))
                    {
                    }
                    std::this_thread::sleep_for(
                        std::chrono::milliseconds(10));
                    activeThreads.fetch_sub(1);
                });
            pool.SubmitTask(tasks.back());
        }
        pool.AwaitTasks();
    }

    EXPECT_GE(maxActive.load(), 1);
}

TEST(TaskThreadPoolTest, AwaitTasks)
{
    std::atomic<int>          counter{0};
    const int                 taskCount = 50;

    TaskThreadPool_t          pool(4);
    std::vector<Task_t<void>> tasks;
    tasks.reserve(taskCount);

    for (int i = 0; i < taskCount; i++)
    {
        tasks.emplace_back(
            [&]()
            {
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(5));
                counter.fetch_add(1);
            });
        pool.SubmitTask(tasks.back());
    }

    pool.AwaitTasks();

    EXPECT_EQ(counter.load(), taskCount);
}

TEST(TaskThreadPoolTest, SubmitMultipleTasks)
{
    std::atomic<int>         counter{0};
    const int                taskCount = 20;

    TaskThreadPool_t         pool(2);
    std::vector<Task_t<int>> tasks;
    tasks.reserve(taskCount);

    for (int i = 0; i < taskCount; i++)
    {
        tasks.emplace_back([i]() { return i * 2; });
    }

    pool.SubmitMultipleTasks(tasks);
    pool.AwaitTasks();

    int sum = 0;
    for (auto& task : tasks)
    {
        sum += task.getFuture().get();
    }

    EXPECT_EQ(sum, taskCount * (taskCount - 1));
}

TEST(TaskThreadPoolTest, CleanShutdown)
{
    std::atomic<int>          counter{0};

    std::vector<Task_t<void>> tasks;
    {
        TaskThreadPool_t pool(2);
        tasks.reserve(10);
        for (int i = 0; i < 10; i++)
        {
            tasks.emplace_back(
                [&]()
                {
                    std::this_thread::sleep_for(
                        std::chrono::milliseconds(5));
                    counter.fetch_add(1);
                });
            pool.SubmitTask(tasks.back());
        }
        pool.AwaitTasks();
    }
    EXPECT_EQ(counter.load(), 10);
}

TEST(TaskThreadPoolTest, YieldingCallback)
{
    std::atomic<int> counter{0};

    {
        TaskThreadPool_t pool(1);

        auto yieldingCallback = [&]() { std::this_thread::yield(); };

        Task_t<void> task([&]() { counter.store(100); });
        pool.SubmitTask(task, yieldingCallback);
        pool.AwaitTasks();
    }

    EXPECT_EQ(counter.load(), 100);
}

TEST(TaskThreadPoolTest, TaskExecutionOrderNotGuaranteed)
{
    std::vector<int> completionOrder;
    std::mutex       mutex;
    const int        taskCount = 10;

    std::vector<Task_t<void>>
        tasks; // Keep alive until pool destroyed
    {
        TaskThreadPool_t pool(4);
        tasks.reserve(taskCount);

        for (int i = 0; i < taskCount; i++)
        {
            int delay = (taskCount - i) * 2;
            tasks.emplace_back(
                [i, delay, &completionOrder, &mutex]()
                {
                    std::this_thread::sleep_for(
                        std::chrono::milliseconds(delay));
                    std::lock_guard lock(mutex);
                    completionOrder.push_back(i);
                });
            pool.SubmitTask(tasks.back());
        }
        pool.AwaitTasks();
    }

    EXPECT_EQ(static_cast<int>(completionOrder.size()), taskCount);
}
