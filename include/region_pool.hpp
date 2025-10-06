// A pool of memory regions. A pool is just a vector of memory regions.
// However allows access to multi-threading across a large amount of regions.
#include <memory>
#include <memory_region.hpp>
#include <vector>

// All search patterns will append to the region results history.
class MemoryRegionPool
{
  private:
    MemoryRegion                      m_parentRegion;

    std::vector<RegionPropertiesList> m_regionResultsHistory_l;

    std::vector<MemoryRegion>         m_childRegions_l;

  public:
    void snapshotParentRegion();

    void findParentRegionStr(const std::string& str);

    // Must be the absolute value of the pointer;
    // finds all pointers which point to the given memory address.
    void findParentRegionPtr(const uintptr_t& ptr);

    // Requires 2 snapshots.
    void findParentRegionChanges();

    // Requires 2 snapshots.
    void findParentRegionUnchanged();

    void findParentRegionPtrlike();

    void findParentRegionStrlike();

    void findParentRegionStruct(
        const std::vector<StructProperties>& structProperties_l);

  public:
    void snapshotChildRegions();

    void findChildRegionsStr(const std::string& str);

    // Must be the absolute value of the pointer;
    // finds all pointers which point to the given memory address.
    void findChildRegionsPtr(const uintptr_t& ptr);

    // Requires 2 snapshots.
    void findChildRegionsChanged();

    // Requires 2 snapshots.
    void findChildRegionsUnchanged();

    void findChildRegionsPtrlike();

    void findChildRegionsStrlike();

    void findChildRegionsStruct(
        const std::vector<StructProperties>& structProperties_l);

  public:
    void makeRegionsFromLastResults();

    void clearResultsHistory();
};
