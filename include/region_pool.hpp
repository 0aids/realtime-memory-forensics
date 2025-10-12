// A pool of memory regions. A pool is just a vector of memory regions.
// However allows access to multi-threading across a large amount of regions.
#pragma once
#include "memory_region.hpp"
#include "region_properties.hpp"
#include <vector>
#include <functional>

struct SmallRegionCapture {

};

using RegionPoolCallback =
    std::function<void(SmallRegionCapture)>;

// All search patterns will append to the region results history.
class MemoryRegionPool
{
  private:
    // Most recent is at the back.
    std::vector<RegionPropertiesList> m_regionResultsHistory_l;

    // Only represents the current state, which is the back of results history.
    std::vector<MemoryRegion> m_memoryRegions_l;

    // We don really care about the order which we get returned in anyways;
    // Runs a threadpool to complete all of the smaller regions
    // Takes in a callback function which receives a memory region; we can
    // use that memory region to do whatever the fuck we want.
    void threadSmallerRegions(RegionPoolCallback rpcb);

    // Runs the larger regions sequentially, making use of the internal
    // threading mechanism.
    // Takes in a callback function which receives a memory region; we can
    // use that memory region to do whatever the fuck we want.
    void threadLargerRegions(RegionPoolCallback rpcb);

  public:
    void findStrRegions(const std::string& str);

    // Must be the absolute value of the pointer;
    // finds all pointers which point to the given memory address.
    void findPtrRegions(const uintptr_t& ptr);

    // Requires 2 snapshots.
    void findChangedRegions(size_t numBytes);

    // Requires 2 snapshots.
    void findUnchangedRegions(size_t numBytes);

    void findPtrlikeRegions();

    void findStrlikeRegions();

    void findRegionsStruct(
        const std::vector<StructProperties>& structProperties_l);

  public:
    void makeRegionsFromLastResults();

    void snapshotRegions();

    void clearResultsHistory();
};
