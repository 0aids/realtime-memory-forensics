#ifndef test_helpers_hpp_INCLUDED
#define test_helpers_hpp_INCLUDED
// A collection of helpers for testing.
#include <chrono>
#include "rmf/logging/logging.hpp"
#include "rmf/utils/expect.hpp"
#include <concepts>
#include <functional>
#include <iterator>
#include <span>
#include <cstdint>
#include <thread>
#include <utility>
#include <sys/prctl.h>
#include <sys/signal.h>
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
            t.run();
        };
    }

    using TestProgramFunc = void (*)();

    // Comptime generate a program with test data during comptime.
    // Polls the features every 10ms.
    template <typename... Features>
    consteval auto createTestProgram(Features&&... features);

    // A basic string buffer for use in the test program.
    template <size_t N>
    struct StaticStringBuffer
    {
        volatile const char buffer[N] = {};
        void                setup() const {};
        void                run() const {};
    };
    template <size_t N>
    StaticStringBuffer(const char (&)[N]) -> StaticStringBuffer<N>;

    template <typename Number, Number Value>
    struct StaticNumberBuffer
    {
        volatile const Number num = Value;
        void                  setup() const {};
        void                  run() const {};
        consteval explicit StaticNumberBuffer() = default;
    };

    // Just prints.
    struct TestFeature
    {
        void setup() const
        { rmf_Ok("setup"); };
        void run() const { rmf_Ok("run!") };
    };

    pid_t forkFunc(auto&& func);

    // A buffer that can be constructed and then tested against.
    // To be used with operations, not for anything else.
    // Optionally aligns pushed structs for realism.
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
        Utils::ErrU<bool> pushAligned(const T& value);

        template <typename T>
        Utils::ErrU<bool> pushUnaligned(const T& value);

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

    // Comptime generate a program with test data during.
    template <typename... Features>
    consteval auto createTestProgram(Features&&... features)
    {
        return [... features = std::forward<Features>(features)]()
        {
            ([&]() { features.setup(); }(), ...);
            while (true)
            {
                using namespace std::chrono_literals;
                std::this_thread::sleep_for(10ms);
                // For some reason this fails to compile under g++?
                // But compiles perfectly fine under clang?
                ([&]() { features.run(); }(), ...);
            }
        };
    }
    pid_t forkFunc(auto&& func)
    {
        using namespace std::chrono_literals;
        pid_t pid = fork();
        if (pid < 0)
            throw std::runtime_error("Failed to fork test process!");
        else if (pid > 0)
        {
            std::this_thread::sleep_for(100ms);
            return pid;
        }

        // Set it so we die on parent process death.
        if (prctl(PR_SET_PDEATHSIG, SIGKILL) == -1)
        {
            perror("prctl failed");
            exit(EXIT_FAILURE);
        }

        func();
        exit(127);
    }

    // Pushes a type into the buffer, ensuring alignment.
    template <typename T>
    Utils::ErrU<bool> TestBuffer::pushAligned(const T& value)
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
    template <typename T>
    Utils::ErrU<bool> TestBuffer::pushUnaligned(const T& value)
    {
        constexpr size_t size    = sizeof(T);
        auto             oldHead = m_head;
        rmf_retErr(tryReplaceHead(m_head + size));
        memcpy(oldHead.base(), &value, size);
        return true;
    }
}

#endif // test_helpers_hpp_INCLUDED
