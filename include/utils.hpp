#ifndef utils_hpp_INCLUDED
#define utils_hpp_INCLUDED
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
    template <typename T, size_t Size>
    class SPMCQueueNonOwning
    {
      private:
        std::array<T, Size>   m_data;
        std::atomic<uint64_t> m_produceIndex =
            0; // Represents the next available index.
        std::atomic<uint64_t> m_consumeIndex =
            0; // represents the next fillable index.
        // The consume index is greater than the produce index by 1 when full.

      public:
          // The waiting threads always wait for a different value to what
          // it actually is, so only notify_one needs to be called.
          static constexpr uint8_t notifierDefaultValue = 0;
          static constexpr uint8_t notifierWakeValue = 255;
        std::atomic<uint8_t> notifier = notifierDefaultValue;
        // keep it at zero and just use notify_one or notify_all to wake threads.
        static constexpr size_t size = Size;

        // Increments the notifiers for sleeping threads using atomic::wait
        bool             enqueue(const T& value) {
            uint64_t produceIndex = m_produceIndex.load(std::memory_order_relaxed);
            uint64_t consumeIndex = m_consumeIndex.load(std::memory_order_acquire);
            if (produceIndex - consumeIndex > size) {
                // Queue is full
                return false;
            }
            m_data[produceIndex % size] = value;
            m_produceIndex.store(produceIndex + 1, std::memory_order_release);
            notifier.notify_one();

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

            if (produceIndex == consumeIndex) return std::nullopt;

            // Weak because spurious failures are not the end of the world, and because
            // I don't understand memory ordering for strong.
            if (!m_consumeIndex.compare_exchange_weak(consumeIndex, consumeIndex+1, 
                std::memory_order_release
            )) return std::nullopt;

            return m_data[consumeIndex];
        }
    };
}

#endif // utils_hpp_INCLUDED
