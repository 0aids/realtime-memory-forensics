#ifndef utils_hpp_INCLUDED
#define utils_hpp_INCLUDED
#include <chrono>
#include "logger.hpp"
#include <chrono>
#include <optional>
#include <atomic>
#include <semaphore>
#include <string_view>
#include <tuple>
#include <array>
#include <cstddef>
#include <cstdint>
#include "types.hpp"

namespace rmf::utils
{
    constexpr size_t d_defaultQueueSize = 1 << 18;
    // Define a concept for STL containers
    template <typename T>
    concept IsContainer = requires(T t) {
        // Type must have an iterator type
        typename T::iterator;

        // Must have a begin() method returning an iterator
        { t.begin() } -> std::same_as<typename T::iterator>;

        // Must have an end() method returning an iterator
        { t.end() } -> std::same_as<typename T::iterator>;
    };

    template <typename T>
    struct typePrinter;

    template <typename...>
    struct alwaysFalse : std::false_type
    {
    };

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
    // buffer and catches the tail, there might be a chance that the data read by a task becomes
    // rewritten. Currently not possible because the queue size is massive, but who knows in the future.
    template <typename T>
    class SPMCQueue
    {
      private:
        alignas(64) std::vector<T> m_data;
        alignas(64) std::atomic<uint64_t> m_produceIndex =
            0; // Represents the next available index.
        alignas(64) std::atomic<uint64_t> m_consumeIndex =
            0; // represents the next fillable index.
        // The consume index is greater than the produce index by 1 when full.
        std::counting_semaphore<rmf::utils::d_defaultQueueSize> m_semaphore{0};

      public:
        const size_t size;
        SPMCQueue(size_t _size) : size(_size)
        {
            m_data.resize(size);
        }

        bool enqueue(const T& value)
        {
            uint64_t produceIndex =
                m_produceIndex.load(std::memory_order_relaxed);
            uint64_t consumeIndex =
                m_consumeIndex.load(std::memory_order_acquire);
            // We are getting wrapped...
            if (produceIndex - consumeIndex > size - 12)
            {
                // Queue is full
                rmf_Log(rmf_Warning,
                        "Unable to enqueue, queue is full!");
                rmf_Log(rmf_Warning,
                        "Current size: " << produceIndex - consumeIndex - 2);
                return false;
            }
            m_data[produceIndex % size] = value;
            m_produceIndex.store(produceIndex + 1,
                                 std::memory_order_release);
            rmf_Log(rmf_Debug, "Enqueued, notifying one...");
            rmf_Log(rmf_Debug,
                    "Last indices were: Consumer - "
                        << consumeIndex << ", Producer - "
                        << produceIndex + 1);
            m_semaphore.release();

            return true;
       }

        bool empty()
        {
            uint64_t produceIndex =
                m_produceIndex.load(std::memory_order_acquire);
            uint64_t consumeIndex =
                m_consumeIndex.load(std::memory_order_acquire);
            return produceIndex == consumeIndex;
        }

        std::optional<T> tryDequeue()
        {
            if (!m_semaphore.try_acquire())
            {
                // rmf_Log(rmf_Verbose,
                //         "Semaphore has no releases"
                //         );
                return std::nullopt;
            }

            uint64_t consumeIndex =
                m_consumeIndex.fetch_add(1, std::memory_order_acquire);

            rmf_Log(rmf_Debug, "Successful dequeue.");
            rmf_Log(rmf_Debug,
                    "Last indices were: Consumer - " << consumeIndex);

            return m_data[consumeIndex % size];
        }

        template< class Rep, class Period >
        std::optional<T> tryDequeueFor(std::chrono::duration<Rep, Period> duration)
        {
            if (!m_semaphore.try_acquire_for(duration))
            {
                // rmf_Log(rmf_Verbose,
                //         "Semaphore has no releases"
                //         );
                return std::nullopt;
            }

            uint64_t consumeIndex =
                m_consumeIndex.fetch_add(1, std::memory_order_acquire);

            rmf_Log(rmf_Debug, "Successful dequeue.");
            rmf_Log(rmf_Debug,
                    "Last indices were: Consumer - " << consumeIndex);

            return m_data[consumeIndex % size];
        }
    };

    std::string PidToMapsString(const pid_t pid);
    rmf::types::MemoryRegionPropertiesVec
    ParseMaps(const std::string fullPath, pid_t pid);

    rmf::types::MemoryRegionPropertiesVec
    FilterMinSize(const rmf::types::MemoryRegionPropertiesVec& other,
                  const uintptr_t minSize);
    rmf::types::MemoryRegionPropertiesVec
    FilterMaxSize(const rmf::types::MemoryRegionPropertiesVec& other,
                  const uintptr_t maxSize);

    // By exact name
    rmf::types::MemoryRegionPropertiesVec
    FilterName(const rmf::types::MemoryRegionPropertiesVec& other,
               const std::string_view&                      string);

    // By containing the string inside the name
    rmf::types::MemoryRegionPropertiesVec FilterContainsName(
        const rmf::types::MemoryRegionPropertiesVec& other,
        const std::string_view&                      string);

    // Exact match of perms
    rmf::types::MemoryRegionPropertiesVec
    FilterPerms(const rmf::types::MemoryRegionPropertiesVec& other,
                const std::string_view&                      perms);

    // Has perm/s, but may also have other ones.
    rmf::types::MemoryRegionPropertiesVec
    FilterHasPerms(const rmf::types::MemoryRegionPropertiesVec& other,
                   const std::string_view& perms);

    // Does not have perm/s, essentially the inverse of hasPerms.
    rmf::types::MemoryRegionPropertiesVec
    FilterNotPerms(const rmf::types::MemoryRegionPropertiesVec& other,
                   const std::string_view& perms);

    rmf::types::Perms ParsePerms(const std::string_view& perms);

    rmf::types::MemoryRegionPropertiesVec BreakIntoChunks(
        const rmf::types::MemoryRegionPropertiesVec& other,
        uintptr_t chunkSize, uintptr_t ovelapSize = 0);

    rmf::types::MemoryRegionPropertiesVec
    BreakIntoChunks(const rmf::types::MemoryRegionProperties& other,
                    uintptr_t chunkSize, uintptr_t ovelapSize = 0);

    rmf::types::MemoryRegionPropertiesVec
    CompressNestedMrpVec(const std::vector<types::MemoryRegionPropertiesVec> &mrpvecVec);
}

#endif // utils_hpp_INCLUDED
