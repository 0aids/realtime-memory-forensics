#pragma once
#include "globals.hpp"
#include "log.hpp"
#include "region_properties.hpp"
#include <string>
#include <unordered_map>

namespace MemoryAnalysis {

class MemoryMap {
private:
  const pid_t m_pid;
  const std::string m_mapLocation;
  Properties::RegionPropertiesList m_regionProperties_l;
  std::unordered_map<std::string, size_t> m_nameToIndex_l;

public:
  // Supply a pid and it will generate the mapLocation
  MemoryMap(pid_t pid);

  // In the case you want to read from a custom map
  MemoryMap(const std::string &mapLocation);

  e_ErrorTypes readMaps();

  Properties::RegionPropertiesList getPropertiesList();

  Properties::MemoryRegionProperties &operator[](const size_t &index);

  Properties::MemoryRegionProperties &operator[](const std::string &name);

  const Properties::MemoryRegionProperties &
  operator[](const size_t &index) const;

  const Properties::MemoryRegionProperties &
  operator[](const std::string &name) const;
};
} // namespace MemoryAnalysis
