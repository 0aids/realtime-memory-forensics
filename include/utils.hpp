#ifndef utils_hpp_INCLUDED
#define utils_hpp_INCLUDED
#include "logger.hpp"
#include <chrono>
#include <optional>
#include <atomic>
#include <tuple>
#include <array>
#include <cstddef>
#include <cstdint>

namespace rmf::utils
{

    // Specialization for function types R(Args...)
    template <typename T>
    struct getArgumentTypes;

    template <typename T, typename... Args>
    struct getArgumentTypes<T(Args...)>
    {
        using argsTuple = std::tuple<Args...>;
    };

    // I swear to god if you unscope tasks before they are done, I am going to completely bomb you.
    //
    // TODO: Fix using shared pointers or something that notifies this guy that the references are
    // invalid!!!
    // BUG: In the very off-chance that we get a shit tonne of tasks such that it wraps around the ring
    // buffer and catches the tail, there might be a chance that the data which is read by 
    template <typename T>
    class SPMCQueueNonOwning
    {
      private:
        alignas(64) std::vector<T>   m_data;
        alignas(64) std::atomic<uint64_t> m_produceIndex =
            0; // Represents the next available index.
        alignas(64) std::atomic<uint64_t> m_consumeIndex =
            0; // represents the next fillable index.
        // The consume index is greater than the produce index by 1 when full.

      public:
          SPMCQueueNonOwning(size_t _size): size(_size) {
              m_data.resize(size);
          }
          // The waiting threads always wait for a different value to what
          // it actually is, so only notify_one needs to be called.
        std::atomic<uint8_t> notifier = 0;
        // keep it at zero and just use notify_one or notify_all to wake threads.
        const size_t size;

        // Increments the notifiers for sleeping threads using atomic::wait
        bool             enqueue(const T& value) {
            uint64_t produceIndex = m_produceIndex.load(std::memory_order_relaxed);
            uint64_t consumeIndex = m_consumeIndex.load(std::memory_order_acquire);
            if (produceIndex - consumeIndex > size) {
                // Queue is full
                rmf_Log(rmf_Warning, "Unable to enqueue, queue is full!");
                return false;
            }
            m_data[produceIndex % size] = value;
            m_produceIndex.store(produceIndex + 1, std::memory_order_release);
            notifier++;
            notifier.notify_one();
            rmf_Log(rmf_Debug, "Enqueued, notifying one...");
            rmf_Log(rmf_Debug, "Last indices were: Consumer - " << consumeIndex << ", Producer - " << produceIndex + 1);

            return true;
        }

        bool             empty() {
            uint64_t produceIndex = m_produceIndex.load(std::memory_order_acquire);
            uint64_t consumeIndex = m_consumeIndex.load(std::memory_order_acquire);
            return produceIndex == consumeIndex;
        }

        std::optional<T> tryDequeue() {
            uint64_t produceIndex = m_produceIndex.load(std::memory_order_acquire);
            uint64_t consumeIndex = m_consumeIndex.load(std::memory_order_acquire);

            // queue is empty.
            if (produceIndex == consumeIndex) 
            {
                rmf_Log(rmf_Debug, "Failed dequeue: Queue is empty...");
                return std::nullopt;
            }

            // Weak because spurious failures are not the end of the world, and because
            // I don't understand memory ordering for strong.
            if (!m_consumeIndex.compare_exchange_weak(consumeIndex, consumeIndex+1, 
                std::memory_order_release
            )) 
            {
                rmf_Log(rmf_Debug, "Failed dequeue: Task was stolen by another thread.");
                return std::nullopt;
            }
            rmf_Log(rmf_Debug, "Successful dequeue.");
            rmf_Log(rmf_Debug, "Last indices were: Consumer - " << consumeIndex + 1 << ", Producer - " << produceIndex);

            return m_data[consumeIndex % size];
        }
    };
}

#endif // utils_hpp_INCLUDED
