#include <gtest/gtest.h>
#include <print>
#include <rmf/logging/logging.hpp>
#include <rmf/rmf.hpp>
#include <rmf/utils/expect.hpp>
#include <rmf/utils/threadpool.hpp>
#include <chrono>
#include <thread>

namespace mf  = RealtimeMemoryForensics;
namespace mfl = mf::Logging;
namespace mfu = mf::Utils;
using namespace std;

namespace
{
    using SPMCQueueInt = mfu::Detail::SPMCQueue<int>;
}

TEST(threadpool, SPMCQueue_tryEnqueueFailsWhenFull)
{
    constexpr size_t queueSize = 4;
    SPMCQueueInt     queue(queueSize);

    EXPECT_TRUE(queue.empty());

    for (size_t i = 0; i < queueSize - 1; i++)
    {
        EXPECT_TRUE(queue.tryEnqueue(static_cast<int>(i)));
        EXPECT_FALSE(queue.empty());
    }

    EXPECT_FALSE(queue.tryEnqueue(99));
    EXPECT_FALSE(queue.empty());
}

TEST(threadpool, SPMCQueue_empty)
{
    SPMCQueueInt queue(10);
    EXPECT_TRUE(queue.empty());

    queue.tryEnqueue(42);
    EXPECT_FALSE(queue.empty());

    auto result = queue.tryDequeue();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 42);
    EXPECT_TRUE(queue.empty());
}

TEST(threadpool, SPMCQueue_tryDequeueReturnsNullopt)
{
    SPMCQueueInt queue(10);
    EXPECT_TRUE(queue.empty());

    auto result = queue.tryDequeue();
    EXPECT_FALSE(result.has_value());
}

TEST(threadpool, SPMCQueue_tryDequeueForTimeout)
{
    SPMCQueueInt queue(10);
    EXPECT_TRUE(queue.empty());

    auto result = queue.tryDequeueFor(chrono::milliseconds(10));
    EXPECT_FALSE(result.has_value());
}

TEST(threadpool, ThreadPool_validFuture)
{
    mfu::ThreadPool tp(2, 100);

    auto            future = tp.pushTask(+[]() { return 42; });
    tp.awaitTasks();

    EXPECT_EQ(future.get(), 42);
}

TEST(threadpool, ThreadPool_awaitTasksBlocks)
{
    mfu::ThreadPool tp(2, 100);
    atomic<int>     counter{0};

    for (int i = 0; i < 10; i++)
    {
        tp.pushTask(
            [&counter]() -> int
            {
                std::this_thread::sleep_for(1ms);
                counter++;
                return 0;
            });
    }

    tp.awaitTasks();
    EXPECT_EQ(counter, 10);
}

TEST(threadpool, ThreadPool_destructorWaits)
{
    std::atomic<bool> started{false};
    std::atomic<bool> finished{false};

    {
        mfu::ThreadPool tp(1, 100);
        tp.pushTask(
            [&started, &finished]()
            {
                started = true;
                std::this_thread::sleep_for(10ms);
                finished = true;
            });
        tp.awaitTasks();
    }

    EXPECT_TRUE(started);
    EXPECT_TRUE(finished);
}

TEST(threadpool, basicTest)
{
    size_t numThreads = std::thread::hardware_concurrency() / 2;
    println("Num threads {}", numThreads);
    mfu::ThreadPool tp(numThreads, 30);
    auto            lambdaMaker = [](size_t delay)
    {
        return [d = delay]()
        {
            // println("Delay {}", d);
            std::this_thread::sleep_for(chrono::milliseconds(d));
            return d;
        };
    };
    const size_t                     length = 200;
    std::vector<std::future<size_t>> futures;
    futures.reserve(length);
    for (size_t i = length; i > 0; i--)
    {
        futures.push_back(tp.pushTask(lambdaMaker(i)));
    }
    tp.awaitTasks();
    println("Finished awaiting tasks!");
    for (auto& f : futures)
    {
        cout << f.get();
    }
}

TEST(threadpool, ThreadPool_taskWithArguments)
{
    mfu::ThreadPool tp(2, 100);

    auto            future =
        tp.pushTask(+[](int x, int y) { return x + y; }, 10, 20);
    tp.awaitTasks();

    EXPECT_EQ(future.get(), 30);
}

TEST(threadpool, ThreadPool_variousReturnTypes)
{
    mfu::ThreadPool tp(2, 100);

    auto            futureInt  = tp.pushTask(+[]() { return 42; });
    auto            futureVoid = tp.pushTask(+[]() { return; });
    auto futureStr = tp.pushTask(+[]() { return string("hello"); });

    tp.awaitTasks();

    EXPECT_EQ(futureInt.get(), 42);
    futureVoid.get();
    EXPECT_EQ(futureStr.get(), "hello");
}
