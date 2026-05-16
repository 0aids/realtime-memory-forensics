#include "rmf/test_helpers.hpp"
#include "rmf/utils/expect.hpp"
#include <cassert>
#include <cstdint>

namespace rmf::Tests
{
    TestBuffer::TestBuffer(size_t size, bool zeroed, size_t alignment)
    {
        // + 2* alignment to ensure that there's enough space for alignment.
        // The vector itself won't be aligned (cannot guarantee this)
        if (zeroed)
            m_buffer = decltype(m_buffer)(size + 2 * alignment, 0);
        else
            m_buffer = decltype(m_buffer)(size + 2 * alignment);
        auto iter = m_buffer.begin();

        // TODO: Replace with helper?
        // Logic is already implemented somewhere else but uses errors.
        size_t diff = (uintptr_t)iter.base() % alignment;
        if (diff != 0)
        {
            iter += alignment - diff;
            assert(m_buffer.end() - iter > 0);
        }

        m_alignedSpan = std::span<uint8_t>(iter, iter + size);
        m_head        = m_alignedSpan.begin();
        assert((uintptr_t)m_head.base() % alignment == 0);
    }

    Utils::ErrU<bool> TestBuffer::tryReplaceHead(iter newIter)
    {
        if (newIter - m_alignedSpan.end() > 0)
            return rmf_mkErr(Utils::ErrorEnum::TestBufferOverflow);
        m_head = newIter;
        return true;
    }

    TestBuffer TestBuffer::make(size_t size)
    {
        return TestBuffer(size, false, sizeof(void*));
    }

    TestBuffer TestBuffer::makeZeroed(size_t size)
    {
        return TestBuffer(size, true, sizeof(void*));
    }

    std::span<uint8_t> TestBuffer::alignedBuffer()
    {
        return m_alignedSpan;
    }

    Utils::ErrU<bool> TestBuffer::pushPadding(size_t numBytes)
    {
        rmf_retErr(tryReplaceHead(m_head + numBytes));
        return true;
    }

    std::vector<uint8_t> TestBuffer::moveBuffer()
    {
        std::vector<uint8_t> toReturn{};
        std::swap(toReturn, m_buffer);
        // ignore warning here.
        // Attempting to ensure that the buffer we receive is properly aligned.
        return std::move(toReturn);
    }

    TestBuffer::citer TestBuffer::chead() const
    {
        return m_head;
    }

    TestBuffer::citer TestBuffer::cend() const
    {
        return m_alignedSpan.end();
    }

    TestBuffer::citer TestBuffer::cbegin() const
    {
        return m_alignedSpan.begin();
    }
    size_t TestBuffer::headOffset() const
    {
        return chead() - cbegin();
    }
}
