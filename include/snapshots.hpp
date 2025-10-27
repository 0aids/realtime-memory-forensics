#ifndef snapshots_hpp_INCLUDED
#define snapshots_hpp_INCLUDED

#include "maps.hpp"
#include "threadpool.hpp"
#include <vector>
#include <chrono>
#include <cstring>
#include <chrono>

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
#endif // snapshots_hpp_INCLUDED
