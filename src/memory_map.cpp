#include "memory_map.hpp"
#include "log.hpp"
#include "region_properties.hpp"
#include <cstdint>
#include <cstring>
#include <fstream>
#include <sched.h>
#include <string>

static inline std::string pidToMapLocation(pid_t pid) {
    return "/proc/" + std::to_string(pid) + "/maps";
}

MemoryMap::MemoryMap(pid_t pid) :
    m_pid(pid), m_mapLocation(pidToMapLocation(pid)) {
    Log(Message, "Constructed with pid: " << pid);
    Log(Message, "Map location: " << m_mapLocation);
};

MemoryMap::MemoryMap(const std::string& mapLocation) :
    m_pid(NO_PID), m_mapLocation(mapLocation) {
    Log(Message, "Constructed with mapLocation: " << mapLocation);
};

// TODO: Ensure checks for failure.
e_ErrorTypes MemoryMap::readMaps() {
    std::ifstream memoryMapFile(this->m_mapLocation);
    std::string   line;
    int           unnamedRegionNumber = 1;

    int           i = 0;
    while (std::getline(memoryMapFile, line)) {
        uintptr_t startAddr, endAddr;
        char      permR, permW, permX, permP;
        char      name[1024] = "";
        sscanf(line.c_str(), "%lx-%lx %c%c%c%c %*s %*s %*s %[^\n]",
               &startAddr, &endAddr, &permR, &permW, &permX, &permP,
               name);

        char permissions[4] = {permR, permW, permX, permP};
        if (strlen(name) == 0) {
            std::string unnamedName = "UnnamedRegion-" +
                std::to_string(unnamedRegionNumber++);
            strncpy(name, unnamedName.c_str(), sizeof(name) - 1);
        }

        MemoryRegionProperties currentRegionProperties(
            std::string(name), startAddr, endAddr - startAddr,
            toPermissionsMask(permissions));
        this->m_regionProperties_l.push_back(currentRegionProperties);

        if (!this->m_nameToIndex_l.contains(name)) {
            this->m_nameToIndex_l.emplace(name, i);
        }
        i++;
    }
    memoryMapFile.close();
    return SUCCESS;
}
bool MemoryMap::hasChanged() {
    std::ifstream memoryMapFile(this->m_mapLocation);
    std::string   line;
    int           unnamedRegionNumber = 1;

    int           i = 0;
    while (std::getline(memoryMapFile, line)) {
        uintptr_t startAddr, endAddr;
        char      permR, permW, permX, permP;
        char      name[1024] = "";
        sscanf(line.c_str(), "%lx-%lx %c%c%c%c %*s %*s %*s %[^\n]",
               &startAddr, &endAddr, &permR, &permW, &permX, &permP,
               name);

        char permissions[4] = {permR, permW, permX, permP};
        if (strlen(name) == 0) {
            std::string unnamedName = "UnnamedRegion-" +
                std::to_string(unnamedRegionNumber++);
            strncpy(name, unnamedName.c_str(), sizeof(name) - 1);
        }

        MemoryRegionProperties currentRegionProperties(
            std::string(name), startAddr, endAddr - startAddr,
            toPermissionsMask(permissions));
        if (currentRegionProperties != m_regionProperties_l[i]) {
            return true;
        }
        i++;
    }
    memoryMapFile.close();
    return false;
}

MemoryRegionProperties& MemoryMap::operator[](const size_t& index) {
    return this->m_regionProperties_l[index];
}

MemoryRegionProperties&
MemoryMap::operator[](const std::string& name) {
    if (!this->m_nameToIndex_l.contains(name)) {
        Log(Message,
            "Error accessing regionName: '"
                << name << "'; Region does not exist");
        exit(1);
    }
    return this->m_regionProperties_l[this->m_nameToIndex_l.at(name)];
}

const MemoryRegionProperties&
MemoryMap::operator[](const size_t& index) const {
    return this->m_regionProperties_l[index];
}

const MemoryRegionProperties&
MemoryMap::operator[](const std::string& name) const {
    if (!this->m_nameToIndex_l.contains(name)) {
        Log(Error,
            "Error accessing regionName: '"
                << name << "'; Region does not exist");
        exit(1);
    }
    return this->m_regionProperties_l[this->m_nameToIndex_l.at(name)];
}

RegionPropertiesList MemoryMap::getPropertiesList() {
    return this->m_regionProperties_l;
}

RegionPropertiesList
MemoryMap::getRegionsWithPermissions(const std::string_view& chars) {
    RegionPropertiesList list{};
    auto                 mask = toPermissionsMask(chars);

    // Going to be slow cause we gotta make copies.
    for (MemoryRegionProperties p : m_regionProperties_l) {
        // Log_f(Debug,
        //       "p.permissions: " << p.permissions
        //                         << "\tmask: " << mask);
        if (p.permissions == mask) {
            list.push_back(p);
        }
    }
    return list;
}
RegionPropertiesList
MemoryMap::getRegionsWithPermissions(const PermissionsMask& mask) {
    RegionPropertiesList list{};

    // Going to be slow cause we gotta make copies.
    for (MemoryRegionProperties p : m_regionProperties_l) {
        // Log_f(Debug,
        //       "p.permissions: " << p.permissions
        //                         << "\tmask: " << mask);
        if (p.permissions == mask) {
            list.push_back(p);
        }
    }
    return list;
}
