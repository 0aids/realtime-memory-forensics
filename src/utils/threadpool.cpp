#include "rmf/utils/threadpool.hpp"
#include "rmf/utils/expect.hpp"
#include <atomic>
#include <format>
#include <thread>

namespace mf  = RealtimeMemoryForensics;
namespace mfu = mf::Utils;

void mfu::ThreadPool::threadFunction(
    const std::atomic<bool>&          alive,
    SPMCQueue<std::function<void()>>& queue,
    std::atomic<uint64_t>&            m_numRunning)
{
    using namespace std::chrono_literals;
    while (alive.load(std::memory_order_acquire))
    {
        auto value = queue.tryDequeueFor(1ms);
        if (value.has_value())
        {
            // Ensure that we know exactly when everything is finished.
            m_numRunning.fetch_add(1, std::memory_order_release);
            value.value()();
            m_numRunning.fetch_sub(1, std::memory_order_release);
            continue;
        }
    }
    rmf_Debug("Thread is shutting down!");
}
void mfu::ThreadPool::awaitTasks()
{
    using namespace std::chrono_literals;
    while (!m_queue.empty() || m_numRunning.load() > 0)
        std::this_thread::sleep_for(10ms);
}

mfu::ThreadPool::ThreadPool(size_t numThreads, size_t queueSize) :
    m_queue(queueSize)
{
    for (size_t i = 0; i < numThreads; i++)
    {
        m_threads.emplace_back(threadFunction, std::ref(m_alive),
                               std::ref(m_queue));
        m_threadNames.emplace_back(std::to_string(i + 1));

        pthread_setname_np(m_threads.back().native_handle(),
                           m_threadNames.back().c_str());
    }
}
mfu::ThreadPool::~ThreadPool()
{
    m_alive.store(false, std::memory_order_release);
    for (auto& thread : m_threads)
    {
        thread.join();
    }
}
