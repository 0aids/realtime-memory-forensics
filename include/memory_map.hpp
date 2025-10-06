#pragma once
#include "globals.hpp"
#include "log.hpp"
#include "region_properties.hpp"
#include <string>
#include <utility>

struct MemoryRegionPropertiesDiff
{
  public:
    // Do not do references; What if the reference's lifetime exceeds the actual item?
    const MemoryRegionProperties m_properties1;
    const MemoryRegionProperties m_properties2;
};

class MemoryMapSnapshots : public std::vector<RegionPropertiesList>
{
    // Return's a region properties list containing all the
    // regions that differed.
  public:
    // Prints out changes, attempts to identify the same region via (decreasing confidence):
    //      region with same name and permissions and index of same name.
    //                             (IE second occurrence of libm.so.6)
    //      region with same name and permissions
    //      region with same name and index of same name (very unlikely to change permissions)
    //
    //      unnamed region with same permissions and similar start.
    void compareSnapshots(size_t index1, size_t index2);

    MemoryRegionProperties getLargestRegionFromLastSnapshot();
};

class MemoryMap
{
  private:
    const pid_t       m_pid;
    const std::string m_mapLocation;

  public:
    MemoryMapSnapshots m_mapSnapshots_l;
    // Supply a pid and it will generate the mapLocation
    MemoryMap(pid_t pid);

    // In the case you want to read from a custom map
    MemoryMap(const std::string& mapLocation);

    const RegionPropertiesList& snapshotMaps();

    // Check if the map has changed.
    // bool                    hasChanged();
};
