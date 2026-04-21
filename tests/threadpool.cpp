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
