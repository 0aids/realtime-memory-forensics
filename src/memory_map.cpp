#include "memory_map.hpp"
#include "log.hpp"
#include "region_properties.hpp"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <sched.h>
#include <string>

static inline std::string pidToMapLocation(pid_t pid)
{
    return "/proc/" + std::to_string(pid) + "/maps";
}

MemoryMap::MemoryMap(pid_t pid) :
    m_pid(pid), m_mapLocation(pidToMapLocation(pid))
{
    Log(Message, "Constructed with pid: " << pid);
    Log(Message, "Map location: " << m_mapLocation);
};

MemoryMap::MemoryMap(const std::string& mapLocation) :
    m_pid(NO_PID), m_mapLocation(mapLocation)
{
    Log(Message, "Constructed with mapLocation: " << mapLocation);
};

// TODO: Ensure checks for failure.
const RegionPropertiesList& MemoryMap::snapshotMaps()
{
    std::ifstream        memoryMapFile(this->m_mapLocation);
    std::string          line;
    int                  unnamedRegionNumber = 1;

    RegionPropertiesList regionProperties;

    while (std::getline(memoryMapFile, line))
    {
        uintptr_t startAddr, endAddr;
        char      permR, permW, permX, permP;
        char      name[1024] = "";
        sscanf(line.c_str(), "%lx-%lx %c%c%c%c %*s %*s %*s %[^\n]",
               &startAddr, &endAddr, &permR, &permW, &permX, &permP,
               name);

        char permissions[4] = {permR, permW, permX, permP};
        if (strlen(name) == 0)
        {
            std::string unnamedName = "UnnamedRegion-" +
                std::to_string(unnamedRegionNumber++);
            strncpy(name, unnamedName.c_str(), sizeof(name) - 1);
        }

        MemoryRegionProperties currentRegionProperties(
            std::string(name), startAddr, endAddr - startAddr,
            toPermissionsMask(permissions));
        regionProperties.push_back(
            std::move(currentRegionProperties));
    }
    memoryMapFile.close();
    m_mapSnapshots_l.push_back(regionProperties);
    return m_mapSnapshots_l.back();
}

constexpr inline static std::string
shortenName(const std::string& fullname)
{
    if (fullname.length() < 20)
    {
        return fullname;
    }
    size_t lastSlash = fullname.find_last_of('/');
    if (lastSlash != std::string::npos)
    {
        return fullname.substr(lastSlash + 1);
    }
    return fullname;
}

void MemoryMapSnapshots::compareSnapshots(size_t index1,
                                          size_t index2)
{
    if (index1 >= this->size() || index2 >= this->size())
    {
        Log(Error,
            "Invalid indices, number of elements is: "
                << this->size());
    }
    const RegionPropertiesList& m1 = (*this)[index1];
    const RegionPropertiesList& m2 = (*this)[index2];
    // Just do direct comparison for now.
    size_t minSize = std::min(m1.size(), m2.size());
    char   m1orm2 =
        0; // 0 if equal length, -1 if m2 is larger, 1 if m1 is larger.
    if (minSize < m1.size())
    {
        m1orm2 = 1;
        Log(Message,
            "Index2's number of regions is less than Index1's")
    }
    else if (minSize < m2.size())
    {
        m1orm2 = -1;
        Log(Message,
            "Index1's number of regions is less than Index2's")
    }
    Log(Message, "Index1's size: " << m1.size());
    Log(Message, "Index2's size: " << m2.size());
    for (size_t i = 0; i < minSize; i++)
    {
        const auto& m1Val = m1[i];
        const auto& m2Val = m2[i];

        Log_f(Message,
              "m1 name: " << shortenName(m1Val.parentRegionName)
                          << "\tm2 name: "
                          << shortenName(m2Val.parentRegionName)
                          << "\tisDifferent: "
                          << ((m1Val.parentRegionName ==
                               m2Val.parentRegionName) ?
                                  "false" :
                                  "true"));

        Log_f(Message,
              std::hex << std::showbase
                       << "m1 start: " << m1Val.parentRegionStart
                       << "\t\tm2 start: " << m2Val.parentRegionStart
                       << "\t\tisDifferent: "
                       << ((m1Val.parentRegionStart ==
                            m2Val.parentRegionStart) ?
                               "false" :
                               "true"));
        Log_f(Message,
              std::hex << std::showbase
                       << "m1 size: " << m1Val.parentRegionSize
                       << "\t\tm2 size: " << m2Val.parentRegionSize
                       << "\t\tisDifferent: "
                       << ((m1Val.parentRegionSize ==
                            m2Val.parentRegionSize) ?
                               "false" :
                               "true"));
        Log_f(Message,
              "m1 perms: "
                  << permissionsMaskToString(m1Val.permissions)
                  << "\t\tm2 perms: "
                  << permissionsMaskToString(m2Val.permissions)
                  << "\t\tisDifferent: "
                  << ((m1Val.permissions == m2Val.permissions) ?
                          "false" :
                          "true"));
        Log_f(Message, "");
        Log_f(Message, "");
    }
}
MemoryRegionProperties
MemoryMapSnapshots::getLargestRegionFromLastSnapshot()
{
    size_t largestInd = 0;
    for (size_t i = 1; i < back().size(); i++)
    {
        if (this->back()[i].regionSize >
            this->back()[largestInd].regionSize)
        {
            largestInd = i;
        }
    }
    return this->back()[largestInd];
}
