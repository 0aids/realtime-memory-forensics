#ifndef threadpool_hpp_INCLUDED
#define threadpool_hpp_INCLUDED
#include <chrono>
#include <cstdint>
#include <cstddef>
#include <future>
#include <ranges>
#include <thread>
#include <type_traits>
#include <vector>
#include <atomic>
#include <semaphore>
#include <functional>
#include "rmf/logging/logging.hpp"
#include "rmf/utils/expect.hpp"

namespace RealtimeMemoryForensics::Utils
{
    namespace Detail
    {
        template <typename T, ptrdiff_t MaxThreads = 2 << 10>
        class SPMCQueue
        {
          private:
            alignas(64) std::vector<T> m_data;

            // Represents the next available index.
            alignas(64) std::atomic<uint64_t> m_produceIndex = 0;

            // represents the next index that is guaranteed to be done.
            alignas(64) std::atomic<uint64_t> m_consumeCommitIndex =
                0;

            // represents the next index that can be claimed.
            // The consume index is greater than the produce index by 1 when full.
            alignas(64) std::atomic<uint64_t> m_consumeClaimIndex = 0;
            std::counting_semaphore<MaxThreads> m_semaphore{0};

          public:
            uint64_t     getConsumeIndex() const;

            const size_t size;
            SPMCQueue(size_t _size);

            bool   tryEnqueue(T&& value);

            void   enqueue(T&& value);

            bool   empty();

            Opt<T> tryDequeue();

            template <class Rep, class Period>
            Opt<T> tryDequeueFor(
                std::chrono::duration<Rep, Period> duration);
        };
    }

    class ThreadPool
    {
      private:
        Detail::SPMCQueue<std::move_only_function<void()>> m_queue;
        std::vector<std::thread> m_threads     = {};
        std::vector<std::string> m_threadNames = {};
        std::atomic<bool>        m_alive       = true;
        std::atomic<uint64_t>    m_numRunning  = 0;

        static void              threadFunction(
            const std::atomic<bool>&                            alive,
            Detail::SPMCQueue<std::move_only_function<void()>>& queue,
            std::atomic<uint64_t>& m_numRunning);

      public:
        constexpr static size_t DefaultQueueSize = 2 << 20;

        // A task is a function wrapper, which holds a promise and a void function that fulfills the promise.
        // This is a packaged task?
        template <typename Func, typename... Args,
                  typename ReturnType =
                      std::invoke_result_t<Func&&, Args&&...>>
        std::future<ReturnType> pushTask(Func task, Args&&... args);

        void                    awaitTasks();

        ThreadPool(size_t numThreads,
                   size_t queueSize = DefaultQueueSize);
        ~ThreadPool();
    };
}

namespace RealtimeMemoryForensics::Utils::Detail
{
    template <typename T, ptrdiff_t MaxThreads>
    uint64_t SPMCQueue<T, MaxThreads>::getConsumeIndex() const
    { return m_consumeCommitIndex.load(); }

    template <typename T, ptrdiff_t MaxThreads>
    SPMCQueue<T, MaxThreads>::SPMCQueue(size_t _size) : size(_size)
    { m_data.resize(size); }

    template <typename T, ptrdiff_t MaxThreads>
    bool SPMCQueue<T, MaxThreads>::tryEnqueue(T&& value)
    {
        uint64_t produceIndex =
            m_produceIndex.load(std::memory_order_relaxed);
        uint64_t consumeIndex =
            m_consumeCommitIndex.load(std::memory_order_acquire);
        // Queue is full (empty is 0s, full is -1)
        if (produceIndex - consumeIndex >= size - 1)
        {
            rmf_Warning("Unable to enqueue, queue is full!");
            rmf_Warning("Current size: {}",
                        produceIndex - consumeIndex - 1);
            return false;
        }
        m_data[produceIndex % size] = std::move(value);
        m_produceIndex.store(produceIndex + 1,
                             std::memory_order_release);
#ifdef LOG_THREADPOOL
        rmf_Debug("Enqueued, notifying one...");
        rmf_Debug("Last indices were: Consumer - {}, Producer - "
                  "{}",
                  consumeIndex, produceIndex + 1);
#endif
        m_semaphore.release();

        return true;
    }

    template <typename T, ptrdiff_t MaxThreads>
    void SPMCQueue<T, MaxThreads>::enqueue(T&& value)
    {
        uint64_t produceIndex =
            m_produceIndex.load(std::memory_order_relaxed);

        uint64_t consumeIndex =
            m_consumeCommitIndex.load(std::memory_order_acquire);

        while (true)
        {
            produceIndex =
                m_produceIndex.load(std::memory_order_relaxed);
            consumeIndex =
                m_consumeCommitIndex.load(std::memory_order_acquire);

            // Queue has space;
            if (produceIndex - consumeIndex < size - 1)
            {
                break;
            }
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(1ms);
        }
        m_data[produceIndex % size] = std::move(value);
        m_produceIndex.store(produceIndex + 1,
                             std::memory_order_release);
#ifdef LOG_THREADPOOL
        rmf_Debug("Enqueued, notifying one...");
        rmf_Debug("Last indices were: Consumer - {}, Producer - "
                  "{}",
                  consumeIndex, produceIndex + 1);
#endif
        m_semaphore.release();
    }

    template <typename T, ptrdiff_t MaxThreads>
    bool SPMCQueue<T, MaxThreads>::empty()
    {
        uint64_t produceIndex =
            m_produceIndex.load(std::memory_order_acquire);
        uint64_t consumeIndex =
            m_consumeCommitIndex.load(std::memory_order_acquire);
        return produceIndex == consumeIndex;
    }

    template <typename T, ptrdiff_t MaxThreads>
    Opt<T> SPMCQueue<T, MaxThreads>::tryDequeue()
    {
        if (!m_semaphore.try_acquire())
        {
            return nopt;
        }

        uint64_t consumeIndex = m_consumeClaimIndex.fetch_add(
            1, std::memory_order_acquire);

        auto data = std::move(m_data[consumeIndex % size]);
        m_consumeCommitIndex.fetch_add(1, std::memory_order_release);
#ifdef LOG_THREADPOOL
        rmf_Debug("Successful dequeue.");
        rmf_Debug("Last indices were: Consumer - {}", consumeIndex);
#endif
        return data;
    }

    template <typename T, ptrdiff_t MaxThreads>
    template <class Rep, class Period>
    Opt<T> SPMCQueue<T, MaxThreads>::tryDequeueFor(
        std::chrono::duration<Rep, Period> duration)
    {
        if (!m_semaphore.try_acquire_for(duration))
        {
            return nopt;
        }

        uint64_t consumeIndex = m_consumeClaimIndex.fetch_add(
            1, std::memory_order_acquire);

        auto data = std::move(m_data[consumeIndex % size]);
        m_consumeCommitIndex.fetch_add(1, std::memory_order_release);
#ifdef LOG_THREADPOOL
        rmf_Debug("Successful dequeue.");
        rmf_Debug("Last indices were: Consumer - {}", consumeIndex);
#endif

        return data;
    }

}
namespace RealtimeMemoryForensics::Utils
{
    // A task is a function wrapper, which holds a promise and a void function that fulfills the promise.
    // Consider having threads capture a slot in the queue, and doesn't move the data out,
    // rather using references to said data to reduce the need for moves.
    template <typename Func, typename... Args, typename ReturnType>
    std::future<ReturnType> ThreadPool::pushTask(Func func,
                                                 Args&&... args)
    {
        std::promise<ReturnType>        promise;
        std::future<ReturnType>         future = promise.get_future();
        std::move_only_function<void()> lambda =
            [func = std::move(func), p = std::move(promise),
             ... args = std::forward<Args>(args)]() mutable
        {
            if constexpr (std::is_same_v<ReturnType, void>)
            {
                func(args...);
                p.set_value();
            }
            else
            {
                p.set_value(func(args...));
            }
        };
        m_queue.enqueue(std::move(lambda));
        return future;
    }
}
#endif // threadpool_hpp_INCLUDED
