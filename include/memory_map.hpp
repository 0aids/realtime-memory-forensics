#pragma once
#include "globals.hpp"
#include "log.hpp"
#include "region_properties.hpp"
#include <string>
#include <utility>

struct MemoryRegionPropertiesDiff {
  public:
    // Do not do references; What if the reference's lifetime exceeds the actual item?
    const MemoryRegionProperties m_properties1;
    const MemoryRegionProperties m_properties2;
};

class MemoryMapSnapshots : public std::vector<RegionPropertiesList> {
    // Return's a region properties list containing all the
    // regions that differed.
  public:
    // This will just print out the changes for now.
    void compareSnapshots(size_t index1, size_t index2);
};

class MemoryMap {
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
