#pragma once
#include "globals.hpp"
#include "log.hpp"
#include "region_properties.hpp"
#include <string>
#include <unordered_map>

class MemoryMap {
  private:
    const pid_t                             m_pid;
    const std::string                       m_mapLocation;
    RegionPropertiesList                    m_regionProperties_l;
    std::unordered_map<std::string, size_t> m_nameToIndex_l;

  public:
    // Supply a pid and it will generate the mapLocation
    MemoryMap(pid_t pid);

    // In the case you want to read from a custom map
    MemoryMap(const std::string& mapLocation);

    e_ErrorTypes readMaps();

    // Check if the map has changed.
    bool                    hasChanged();

    RegionPropertiesList    getPropertiesList();

    MemoryRegionProperties& operator[](const size_t& index);

    MemoryRegionProperties& operator[](const std::string& name);

    const MemoryRegionProperties&
    operator[](const size_t& index) const;

    const MemoryRegionProperties&
         operator[](const std::string& name) const;

    auto operator<=>(const MemoryMap& other) const {
        return this->m_regionProperties_l <=>
            other.m_regionProperties_l;
    }

    // 4 or more chars, or i'm going to have a fit.
    // rwxp, add spaces for empty.
    RegionPropertiesList
    getRegionsWithPermissions(const std::string_view& chars);
    RegionPropertiesList
    getRegionsWithPermissions(const PermissionsMask& mask);
};
