#ifndef snapshots_hpp_INCLUDED
#define snapshots_hpp_INCLUDED

#include "maps.hpp"
#include <cmath>
#include <vector>
#include <chrono>
#include <cstring>
#include <chrono>
class MemorySnapshotSpan;
struct MemorySnapshot
{
  private:
    const std::vector<char> m_data;

  public:
    size_t size() const noexcept
    {
        return m_data.size();
    }

    auto begin() const noexcept
    {
        return m_data.begin();
    }

    auto end() const noexcept
    {
        return m_data.end();
    }

    const char& front() const
    {
        return m_data.front();
    }

    const char& back() const
    {
        return m_data.back();
    }

    const char& at(size_t pos) const
    {
        return m_data.at(pos);
    }

    const char& operator[](size_t pos) const noexcept
    {
        return m_data[pos];
    }

    const char* data() const
    {
        return m_data.data();
    }

    std::span<const char> asSpan() const noexcept
    {
        return std::span<const char>(m_data.data(), m_data.size());
    }

    MemorySnapshotSpan asSnapshotSpan() const noexcept;

    const MemoryRegionProperties   regionProperties;
    const std::chrono::nanoseconds timeCaptured;

    MemorySnapshot(std::vector<char>        data,
                   MemoryRegionProperties   props,
                   std::chrono::nanoseconds time) :
        m_data(std::move(data)), regionProperties(props),
        timeCaptured(time)
    {
    }
};

class MemorySnapshotSpan
{
  private:
    std::span<const char> m_data;

  public:
    MemorySnapshotSpan(const MemorySnapshot &snap) :
        m_data(snap.asSpan()), regionProperties(snap.regionProperties),
        timeCaptured(snap.timeCaptured)
    {
    }
    MemorySnapshotSpan(
        std::span<const char>           span,
        const MemoryRegionProperties&   mrp,
        const std::chrono::nanoseconds& snapshottedTime) :
        m_data(span), regionProperties(mrp),
        timeCaptured(snapshottedTime)
    {
    }
    // Creates a new memory snapshot span with the appropriate
    // Memory Region Properties.
    // The offset is relative to the current span.
    MemorySnapshotSpan subspan(size_t relativeOffset, size_t count)
    {
        MemoryRegionProperties newMrp = regionProperties;
        newMrp.relativeRegionStart += relativeOffset;
        newMrp.relativeRegionSize = count;
        return MemorySnapshotSpan(m_data.subspan(relativeOffset, count), 
                newMrp, timeCaptured);
    }
    size_t size() const noexcept
    {
        return m_data.size();
    }

    auto begin() const noexcept
    {
        return m_data.begin();
    }

    auto end() const noexcept
    {
        return m_data.end();
    }

    const char& front() const
    {
        return m_data.front();
    }

    const char& back() const
    {
        return m_data.back();
    }

    const char& operator[](size_t pos) const noexcept
    {
        return m_data[pos];
    }

    const char* data() const
    {
        return m_data.data();
    }

    const MemoryRegionProperties   regionProperties;
    const std::chrono::nanoseconds timeCaptured;
};


#endif // snapshots_hpp_INCLUDED
