#ifndef MTQueue_hpp_INCLUDED
#define MTQueue_hpp_INCLUDED
#include <array>
#include <vector>
#include <cstddef>
#include <atomic>
#include <semaphore>
#include "logs.hpp"

// std::queue is not thread safe, so we need a basic queue.
template <typename T>
class SPMCQueue
{
  public:
    constexpr static size_t DEFSIZE = 0x80000;

  private:
    std::unique_ptr<std::array<T, DEFSIZE>> m_data = std::make_unique<std::array<T, DEFSIZE>>();
    std::counting_semaphore<DEFSIZE> m_semaphore{0};

    // why does it have to be braces?
    // need to be aligned correctly supposedly.
    // The tail contains the oldest items, the first in.
    alignas(64) std::atomic<size_t> tail = 0;
    // The head contains the most recent items placed in.
    alignas(64) std::atomic<size_t> head = 0;
    // The rule for this one is that if the head == tail, then we
    // are empty, and if the head == tail - 1, then we are full.

    T dequeueInternal()
    {
        using namespace std;
        size_t curTail = tail.fetch_add(1, memory_order_acq_rel);
        T toReturn = std::move((*m_data)[curTail % DEFSIZE]);

        return toReturn;
    }

  public:
    bool enqueue(const T& value)
    {
        using namespace std;
        // Only one thread can change the head
        size_t curHead = head.load(memory_order_relaxed);
        // Ensure that all changed released by other
        // threads are the viewed
        size_t curTail = tail.load(memory_order_acquire);

        if (curHead - curTail >= DEFSIZE)
        {
            Log(Error, "Failed to enqueue, queue is full!");
            return false;
        }

        // Make a copy here
        (*m_data)[curHead % DEFSIZE] = value;

        head.store(curHead + 1, memory_order_release);
        m_semaphore.release();
        // No other values are modified.

        return true;
    }

    std::optional<T> tryDequeue() {
        if (!m_semaphore.try_acquire())
            return std::nullopt;
        return dequeueInternal();
    }

    T blockingDequeue() {
        m_semaphore.acquire();
        return dequeueInternal();
    }

    template <class Rep, class Period>
    std::optional<T> tryDequeueFor(const std::chrono::duration<Rep, Period>& duration) {
        if (!m_semaphore.try_acquire_for(duration)) 
            return std::nullopt;
        return dequeueInternal();
    }


    // May return out of date information.
    size_t size() const noexcept
    {
        size_t curHead = head.load(std::memory_order_acquire);
        size_t curTail = tail.load(std::memory_order_acquire);
        return (curHead - curTail);
    }

    // May return out of date information
    bool isEmpty() const noexcept
    {
        return (head.load(std::memory_order_acquire) == 
            tail.load(std::memory_order_acquire)
                );
    }
};

#endif // MTQueue_hpp_INCLUDED
