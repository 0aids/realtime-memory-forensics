#include "region_properties.hpp"
#include <ctime>
#include <memory>
#include <vector>
#include <chrono>

struct RegionSnapshot : public std::vector<char> {
  private:
    inline static bool failed = false;

  public:
    std::chrono::nanoseconds snapshottedTime;
    // This is to allow the parent memory region
    // to truncate it's memory reading area without
    // breaking previous regions. When comparisons are done
    // We'll read the actual property that it was snapshotted with
    // And this will ensure that the we don't do stupid shit.
    const MemoryRegionProperties regionProperties;

    std::string                  toStr() const;

    // Compare region for a certain comparison size (in bytes).
    // It returns a list of regionProperties with the corresponding changed regions.
    // Only compares the 2 regions if they have the same properties.
    std::vector<MemoryRegionProperties>
    findChangedRegions(const RegionSnapshot& otherRegion,
                       uint32_t              compareSize) const;
    std::vector<MemoryRegionProperties>
    findUnchangedRegions(const RegionSnapshot& otherRegion,
                         uint32_t              compareSize) const;

    // Find a string-like region - a region that contains alphanumeric stuff.
    std::vector<MemoryRegionProperties>
    findStringLikeRegions(const size_t& minLength);
    RegionSnapshot(std::chrono::nanoseconds     time,
                   const MemoryRegionProperties regionProps) :
        snapshottedTime(time), regionProperties(regionProps) {};

    static void resetFailed();
    static bool getFailed();
};

using SP_RegionSnapshot = std::shared_ptr<RegionSnapshot>;

struct SnapshotList_SP : public std::vector<SP_RegionSnapshot> {};

class MemoryRegion {
  public:
    pid_t                  m_pid;
    MemoryRegionProperties m_regionProperties;
    SnapshotList_SP        m_snapshots_l;
    MemoryRegion(MemoryRegionProperties properties, pid_t pid);
    SP_RegionSnapshot getLastSnapshot() const;
    void              snapshot();
};
