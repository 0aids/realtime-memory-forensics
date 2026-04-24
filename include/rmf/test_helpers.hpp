#ifndef test_helpers_hpp_INCLUDED
#define test_helpers_hpp_INCLUDED
// A collection of helpers for testing.
#include <chrono>
#include "rmf/utils/expect.hpp"
#include <concepts>
#include <functional>
#include <iterator>
#include <span>
#include <cstdint>
namespace RealtimeMemoryForensics::Tests
{
    namespace Detail
    {
        using namespace std::chrono;
        using std::same_as;
        // Feature concept - Must provide a .setup(), a .run(), and a .when()
        template <typename T>
        concept Feature = requires(T t) {
            // Setup initialises the feature.
            t.setup();

            // Run runs the feature. This should be done when it is time to run it.
            // It returns the next time it should be run.
            { t.run() } -> same_as<time_point<steady_clock>>;

            // It returns the next time it should be run.
            { t.when() } -> same_as<time_point<steady_clock>>;
        };
        template <std::random_access_iterator T>
        T alignIter(T t, size_t alignment);
    }

    using TestProgramFunc = void (*)();

    // Comptime generate a program with test data during comptime.
    template <Detail::Feature... Features>
    consteval TestProgramFunc createTestProgram();

    // A basic string buffer for use in the test program.
    template <const char* string, size_t N = sizeof(string)>
    struct StaticStringBuffer
    {
        using SelfType       = StaticStringBuffer<string, N>;
        const char buffer[N] = string;
        void       setup();
        std::chrono::time_point<std::chrono::steady_clock> when();
        consteval explicit StaticStringBuffer() = default;

        static_assert(Detail::Feature<SelfType>,
                      "Should conform to feature!");
    };

    // A buffer that can be constructed and then tested against.
    class TestBuffer
    {
      public:
        using iter  = std::span<uint8_t>::iterator;
        using citer = std::span<uint8_t>::const_iterator;

      private:
        std::vector<uint8_t> m_buffer;
        std::span<uint8_t>   m_alignedSpan;
        iter                 m_head;
        TestBuffer(size_t size, bool zeroed, size_t alignment);
        Utils::ErrU<bool> tryReplaceHead(iter newIter);

      public:
        TestBuffer(size_t size) = delete;
        // Guarantees that it's aligned to sizeof(void*)
        // and that the size is actually equal to that size.
        static TestBuffer  make(size_t size);
        static TestBuffer  makeZeroed(size_t size);
        std::span<uint8_t> alignedBuffer();
        citer              chead() const;
        citer              cend() const;
        citer              cbegin() const;
        size_t             headOffset() const;

        // Pushes a type into the buffer, ensuring alignment.
        template <typename T>
        Utils::ErrU<bool> push(const T& value);

        // Push a number of bytes.
        Utils::ErrU<bool>    pushPadding(size_t numBytes);
        std::vector<uint8_t> moveBuffer();
    };
}

namespace RealtimeMemoryForensics::Tests
{
    namespace Detail
    {
        template <std::random_access_iterator T>
        T alignIter(T t, size_t alignment)
        {
            size_t diff = (uintptr_t)t.base() % alignment;
            if (diff != 0)
            {
                t += alignment - diff;
            }
            return t;
        }
    }
    // Comptime generate a program with test data during comptime.
    template <Detail::Feature... Features>
    consteval TestProgramFunc createTestProgram()
    { static_assert(false, "Unimplemented!"); }

    template <const char* string, size_t N>
    void StaticStringBuffer<string, N>::setup()
    { static_assert(false, "Unimplemented!"); }

    template <const char* string, size_t N>
    std::chrono::time_point<std::chrono::steady_clock>
    StaticStringBuffer<string, N>::when()
    { static_assert(false, "Unimplemented!"); }

    // Pushes a type into the buffer, ensuring alignment.
    template <typename T>
    Utils::ErrU<bool> TestBuffer::push(const T& value)
    {
        constexpr size_t alignment = alignof(T);
        constexpr size_t size      = sizeof(T);
        rmf_retErr(
            tryReplaceHead(Detail::alignIter(m_head, alignment)));
        auto oldHead = m_head;
        rmf_retErr(tryReplaceHead(m_head + size));
        memcpy(oldHead.base(), &value, size);
        return true;
    }
}

#endif // test_helpers_hpp_INCLUDED
