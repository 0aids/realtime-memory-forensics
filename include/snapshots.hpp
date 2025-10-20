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
  public:
    struct Builder
    {
        std::vector<char>        data;
        MemoryRegionProperties   regionProperties;
        std::chrono::nanoseconds timeCaptured;

        // Build the snapshot from the non-const values.
        const MemorySnapshot build();

        // Make sure the buffer is the correct size.
        Builder(MemoryRegionProperties properties) :
            regionProperties(properties)
        {
            data.resize(regionProperties.relativeRegionSize);
        }
    };

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

// It's a threadable version, not one supposed to be public facing.
void makeSnapshotCore(MemorySnapshot::Builder& builder,
                      MemoryPartition           partition);

// Single threaded, make snapshot
MemorySnapshot makeSnapshotST(MemoryRegionProperties properties);

// Multi-threaded, makes larger snapshots.
MemorySnapshot makeSnapshotMT(MemoryRegionProperties properties,
                              ThreadPool&            tp);

// If the code for these 2 is nice i will be very happy.
std::vector<MemorySnapshot>
makeLotsOfSnapshotsST(RegionPropertiesList rl);

std::vector<MemorySnapshot>
makeLotsOfSnapshotsMT(RegionPropertiesList rl, ThreadPool& tp);

void findChangedRegionsCore(RegionPropertiesList::Builder& build,
                            uintptr_t compareSize,
                            MemoryPartition mp,
                            const MemorySnapshot& snap1,
                            const MemorySnapshot& snap2);

RegionPropertiesList findChangedRegionsMT(const MemorySnapshot& snap1,
                                          const MemorySnapshot& snap2,
                                          ThreadPool&           tp,
                                          uintptr_t compareSize);

RegionPropertiesList
findChangedRegionsRegionPoolMT(const std::vector<MemorySnapshot> &snaps1,
                             const std::vector<MemorySnapshot> &snaps2, ThreadPool &tp, 
                               uintptr_t compareSize);

#endif // snapshots_hpp_INCLUDED
