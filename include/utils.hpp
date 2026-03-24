#ifndef utils_hpp_INCLUDED
#define utils_hpp_INCLUDED
#include <chrono>
#include <iterator>
#include <ranges>
#include <stdexcept>
#include <utility>
#include "logger.hpp"
#include <chrono>
#include <optional>
#include <atomic>
#include <semaphore>
#include <string_view>
#include <tuple>
#include <cstddef>
#include <cstdint>
#include <type_traits>
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

    template <typename... T>
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

    template <typename T>
    class SPMCQueue
    {
      private:
        alignas(64) std::vector<T> m_data;
        alignas(64) std::atomic<uint64_t> m_produceIndex =
            0; // Represents the next available index.
        alignas(64) std::atomic<uint64_t> m_consumeCommitIndex =
            0; // represents the next index that is guaranteed to be done.
        alignas(64) std::atomic<uint64_t> m_consumeClaimIndex =
            0; // represents the next index that can be claimed.
        // The consume index is greater than the produce index by 1 when full.
        std::counting_semaphore<rmf::utils::d_defaultQueueSize>
            m_semaphore{0};

      public:
        uint64_t getConsumeIndex() const
        {
            return m_consumeCommitIndex.load();
        }

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
                m_consumeCommitIndex.load(std::memory_order_acquire);
            // Queue is full (empty is 0s, full is -1)
            if (produceIndex - consumeIndex >= size - 1)
            {
                rmf_Log(rmf_Warning,
                        "Unable to enqueue, queue is full!");
                rmf_Log(rmf_Warning,
                        "Current size: " << produceIndex -
                                consumeIndex - 1);
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
                m_consumeCommitIndex.load(std::memory_order_acquire);
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

            uint64_t consumeIndex = m_consumeClaimIndex.fetch_add(
                1, std::memory_order_acquire);

            auto data = m_data[consumeIndex % size];
            m_consumeCommitIndex.fetch_add(1,
                                           std::memory_order_release);
            rmf_Log(rmf_Debug, "Successful dequeue.");
            rmf_Log(rmf_Debug,
                    "Last indices were: Consumer - " << consumeIndex);

            return data;
        }

        template <class Rep, class Period>
        std::optional<T>
        tryDequeueFor(std::chrono::duration<Rep, Period> duration)
        {
            if (!m_semaphore.try_acquire_for(duration))
            {
                // rmf_Log(rmf_Verbose,
                //         "Semaphore has no releases"
                //         );
                return std::nullopt;
            }

            uint64_t consumeIndex = m_consumeClaimIndex.fetch_add(
                1, std::memory_order_acquire);

            auto data = m_data[consumeIndex % size];
            m_consumeCommitIndex.fetch_add(1,
                                           std::memory_order_release);
            rmf_Log(rmf_Debug, "Successful dequeue.");
            rmf_Log(rmf_Debug,
                    "Last indices were: Consumer - " << consumeIndex);

            return data;
        }
    };

    std::string PidToMapsString(const pid_t pid);

    rmf::types::MemoryRegionPropertiesVec
    ParseMaps(const std::string fullPath);

    rmf::types::MemoryRegionPropertiesVec getMapsFromPid(pid_t pid);

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
    rmf::types::MemoryRegionPropertiesVec FilterExactPerms(
        const rmf::types::MemoryRegionPropertiesVec& other,
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

    rmf::types::MemoryRegionPropertiesVec CompressNestedMrpVec(
        const std::vector<types::MemoryRegionPropertiesVec>&
            mrpvecVec);

    // Gets actively in memory regions only.
    rmf::types::MemoryRegionPropertiesVec FilterActiveRegions(
        const types::MemoryRegionPropertiesVec& mrpVec, pid_t pid);

    template <std::ranges::range nestedArr>
        requires std::ranges::input_range<
            std::ranges::range_value_t<nestedArr>>
    auto flattenArray(const nestedArr& arr)
    {
        using InnerContainer = std::ranges::range_value_t<nestedArr>;

        InnerContainer result = arr | std::views::join |
            std::ranges::to<InnerContainer>();
        return result;
    }
    types::MemoryRegionProperties
    RestructureMrp(const types::MemoryRegionProperties& mrp,
                   const types::MrpRestructure&         restructure);

    struct SlotKey
    {
        uint32_t    index;
        uint32_t    generation;
        friend bool operator==(const SlotKey& a, const SlotKey& b)
        {
            return a.index == b.index && a.generation == b.generation;
        }
    };

    template <typename T>
    class SlotMap
    {
      private:
        struct Slot
        {
            T        data;
            bool     valid;
            uint32_t generation = 0;

            Slot() : valid(false) {}
            Slot(const T& d, bool v) : data(d), valid(v) {}
        };

        std::vector<SlotKey>
            releasedKeys;        // Acts as a stack for free indices
        std::vector<Slot> slots; // Underlying contiguous storage
        size_t            activeCount =
            0; // Tracks the actual number of valid elements

      public:
        SlotMap()                          = default;
        SlotMap(const SlotMap&)            = default;
        SlotMap(SlotMap&&)                 = default;
        SlotMap& operator=(const SlotMap&) = default;
        SlotMap& operator=(SlotMap&&)      = default;

        ~SlotMap() = default;

        size_t size() const
        {
            return activeCount;
        }

        bool empty() const
        {
            return activeCount == 0;
        }

        size_t capacity() const
        {
            return slots.capacity();
        }

        void reserve(size_t new_cap)
        {
            slots.reserve(new_cap);
        }

        // Equivalent to push_back, but returns a stable key
        SlotKey insert(const T& value)
        {
            SlotKey key;

            if (!releasedKeys.empty())
            {
                // Reuse an old, invalidated slot
                key = releasedKeys.back();
                releasedKeys.pop_back();
                slots[key.index].data  = value;
                slots[key.index].valid = true;
                key.generation = ++slots[key.index].generation;
            }
            else
            {
                // Expand the underlying vector with a new slot
                key = {slots.size(), 0};
                slots.push_back({value, true});
            }

            activeCount++;
            return key;
        }

        // Equivalent to vector.erase(), but takes our custom key
        void erase(SlotKey key)
        {
            if (key.index < slots.size() && slots[key.index].valid)
            {
                slots[key.index].valid = false;
                releasedKeys.push_back(key);
                activeCount--;
            }
        }

        void clear()
        {
            slots.clear();
            releasedKeys.clear();
            activeCount = 0;
        }

        // Unchecked access
        T& operator[](SlotKey key)
        {
            return slots[key.index].data;
        }

        const T& operator[](SlotKey key) const
        {
            return slots[key.index].data;
        }

        bool contains(SlotKey key) const
        {
            if (key.index >= slots.size() ||
                slots[key.index].generation != key.generation ||
                !slots[key.index].valid)
            {
                return false;
            }
            return true;
        }

        SlotKey replace(SlotKey key, const T& data)
        {
            if (!contains(key))
                throw std::out_of_range("Not valid key");
            erase(key);
            return insert(data);
        }

        // Checked access (no throws because I don't use exceptions)
        T& at(SlotKey key)
        {
            if (!contains(key))
                throw std::out_of_range("Not valid key");
            return slots[key.index].data;
        }

        const T& at(SlotKey key) const
        {
            if (!contains(key))
                throw std::out_of_range("Not valid key");
            return slots[key].data;
        }

      public:
        // --- 4. Iterators ---
        template <bool isConst>
        class Iterator
        {
            using vecIter = std::conditional_t<
                isConst, typename std::vector<Slot>::const_iterator,
                typename std::vector<Slot>::iterator>;

          private:
            // We wrap the standard vector iterators
            size_t  index = 0;
            vecIter current;
            vecIter end;

            void    forward()
            {
                ++index;
                ++current;
            }

            // Core logic: Fast-forward past any invalid "holes"
            void forwardToValid()
            {
                while (current != end && !current->valid)
                {
                    forward();
                }
            }

            void backward()
            {
                --index;
                --current;
            }

            void backwardToValid()
            {
                while (index != 0 && !current->valid)
                {
                    backward();
                }
            }

          public:
            // STL type traits (required for standard library compatibility)
            using iterator_category = std::random_access_iterator_tag;
            using value_type =
                std::conditional_t<isConst,
                                   std::pair<const SlotKey, const T>,
                                   std::pair<const SlotKey, T>>;
            using difference_type = std::ptrdiff_t;
            using reference =
                std::conditional_t<isConst,
                                   std::pair<const SlotKey, const T&>,
                                   std::pair<const SlotKey, T&>>;
            using pointer =
                std::conditional_t<isConst,
                                   std::pair<const SlotKey, const T*>,
                                   std::pair<const SlotKey, T*>>;

            // Constructor
            Iterator(size_t index, vecIter start, vecIter end_iter) :
                index(index), current(start), end(end_iter)
            {
                forwardToValid();
            }
            // Required for ranges.
            Iterator() {}

            // Dereferencing
            reference operator*() const
            {
                SlotKey key = {.index      = index,
                               .generation = current->generation};
                return {key, current->data};
            }
            pointer operator->() = delete;

            // Prefix increment (++it)
            Iterator& operator++()
            {
                forward();
                forwardToValid(); // Skip any holes
                return *this;
            }

            Iterator& operator--()
            {
                backward();
                backwardToValid(); // Skip any holes
                return *this;
            }
            // Postfix increment (it++)
            Iterator operator++(int)
            {
                Iterator tmp = *this;
                (*this)++;
                return tmp;
            }

            Iterator operator--(int)
            {
                Iterator tmp = *this;
                (*this)--;
                return *this;
            }

            Iterator& operator+=(difference_type a)
            {
                if (a > 0)
                {
                    for (difference_type i = 0; i < a; i++)
                    {
                        operator++();
                    }
                }
                else
                {
                    for (difference_type i = 0; i < -a; i++)
                    {
                        operator--();
                    }
                }
                return *this;
            }
            Iterator& operator-=(difference_type a)
            {
                return operator+=(-a);
            }

            Iterator operator+(difference_type a) const
            {
                Iterator newIter = *this;
                newIter += a;
                return newIter;
            }

            Iterator operator-(difference_type a) const
            {
                return operator+(-a);
            }

            reference operator[](difference_type a)
            {
                return *(*this + a);
            }

            reference operator[](difference_type a) const
            {
                return *(*this + a);
            }

            friend Iterator operator+(difference_type a,
                                      const Iterator& b)
            {
                return b + a;
            }

            friend difference_type operator-(const Iterator& a,
                                             const Iterator& b)
            {
                if (a.current < b.current)
                {
                    difference_type i = 0;
                    while (a++ != b)
                        i++;
                    return -i;
                }
                else
                {
                    difference_type i = 0;
                    while (b++ != a)
                        i++;
                    return i;
                }
            }

            friend bool operator==(const Iterator& a,
                                   const Iterator& b)
            {
                return a.current == b.current;
            }

            friend bool operator!=(const Iterator& a,
                                   const Iterator& b)
            {
                return a.current != b.current;
            }

            friend bool operator<=(const Iterator& a,
                                   const Iterator& b)
            {
                return a.index <= b.index;
            }

            friend bool operator>=(const Iterator& a,
                                   const Iterator& b)
            {
                return a.index >= b.index;
            }

            friend bool operator>(const Iterator& a,
                                  const Iterator& b)
            {
                return a.index > b.index;
            }
            friend bool operator<(const Iterator& a,
                                  const Iterator& b)
            {
                return a.index < b.index;
            }
        };

        Iterator<false> begin()
        {
            return Iterator<false>(0, slots.begin(), slots.end());
        }

        Iterator<false> end()
        {
            return Iterator<false>(size(), slots.end(), slots.end());
        }
        Iterator<true> cbegin() const
        {
            return Iterator<true>(0, slots.cbegin(), slots.cend());
        }
        Iterator<true> cend() const
        {
            return Iterator<true>(size(), slots.cend(), slots.cend());
        }
        Iterator<true> begin() const
        {
            return cbegin();
        }
        Iterator<true> end() const
        {
            return cend();
        }
    };
}

#endif // utils_hpp_INCLUDED
