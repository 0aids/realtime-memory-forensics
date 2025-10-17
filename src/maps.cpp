#include "maps.hpp"
#include <fstream>
#include <string>
#include "logs.hpp"
#include <algorithm>
#include <cstring>

Perms parsePerms(char r, char w, char x, char p) {
    Perms perms{};
    if (r == 'r') {
        perms |= Perms::READ;
    }
    if (w == 'w') {
        perms |= Perms::WRITE;
    }
    if (x == 'x') {
        perms |= Perms::EXECUTE;
    }
    if (p == 'p') {
        perms |= Perms::PRIVATE;
    }
    if (p == 's') {
        perms |= Perms::SHARED;
    }
    return perms;
}
Perms parsePerms(const std::string_view &s) {
    Perms perms{};
    if (s.contains('r')) {
        perms |= Perms::READ;
    }
    if (s.contains('w')) {
        perms |= Perms::WRITE;
    }
    if (s.contains('x')) {
        perms |= Perms::EXECUTE;
    }
    if (s.contains('p')) {
        perms |= Perms::PRIVATE;
    }
    if (s.contains('s')) {
        perms |= Perms::SHARED;
    }
    return perms;
}

static constexpr std::string pidToMapLocation(pid_t pid)
{
    return "/proc/" + std::to_string(pid) + "/maps";
}

RegionPropertiesList readMapsFromPid(pid_t pid)
{

    Log(Debug, "Reading map from location: " << pidToMapLocation(pid)); 
    std::ifstream        memoryMapFile(pidToMapLocation(pid));
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

        if (strlen(name) == 0)
        {
            std::string unnamedName = "UnnamedRegion-" +
                std::to_string(unnamedRegionNumber++);
            strncpy(name, unnamedName.c_str(), sizeof(name) - 1);
        }

        MemoryRegionProperties m = {
            parsePerms(permR, permW, permX, permP),
            name,
            startAddr,
            endAddr - startAddr,
            0,
            endAddr - startAddr,
            pid,
        };
        Log(Debug, "Found region: \n" << m);
        regionProperties.push_back(std::move(m));
    }
    return regionProperties;
}

RegionPropertiesList
RegionPropertiesList::filterRegionsByMaxSize(const uintptr_t& maxSize)
{
    RegionPropertiesList rl;
    for (size_t i = 0; i < this->size(); i++) {
        if (this->at(i).relativeRegionSize <= maxSize) {
            rl.push_back(this->at(i));
        }
    }

    return rl;
}

RegionPropertiesList
RegionPropertiesList::filterRegionsByMinSize(const uintptr_t& minSize)
{
    RegionPropertiesList rl;
    for (size_t i = 0; i < this->size(); i++) {
        if (this->at(i).relativeRegionSize >= minSize) {
            rl.push_back(this->at(i));
        }
    }

    return rl;
}

RegionPropertiesList RegionPropertiesList::filterRegionsByName(
    const std::string_view& name)
{
    RegionPropertiesList rl;
    for (size_t i = 0; i < this->size(); i++) {
        if (this->at(i).regionName == name) {
            rl.push_back(this->at(i));
        }
    }

    return rl;
}

RegionPropertiesList RegionPropertiesList::filterRegionsBySubName(
    const std::string_view& subName)
{
    RegionPropertiesList rl;
    for (size_t i = 0; i < this->size(); i++) {
        if (this->at(i).regionName.contains(subName)) {
            rl.push_back(this->at(i));
        }
    }

    return rl;
}

RegionPropertiesList RegionPropertiesList::filterRegionsByPerms(
    const std::string_view& perms)
{
    Perms permsToMatch = parsePerms(perms);
    RegionPropertiesList rl;
    for (size_t i = 0; i < this->size(); i++) {
        const Perms &p = this->at(i).perms;
        if (p == permsToMatch) 
            {
                rl.push_back(this->at(i));
            }
    }
    return rl;
}

RegionPropertiesList
RegionPropertiesList::sortRegionsBySize(const bool increasing)
{
    RegionPropertiesList rl;
    std::vector<size_t> indices(this->size());
    std::ranges::iota(indices.begin(), indices.end(), 0);
    std::sort(indices.begin(), indices.end(), 
        [increasing, this](const size_t &a, const size_t &b)
        {
            // Why cannot be constexpr ;_;
            if (increasing) {
                return this->at(a).relativeRegionSize > this->at(b).relativeRegionSize;
            } else {
                return this->at(a).relativeRegionSize < this->at(b).relativeRegionSize;
            }
        }
    );
    for (const auto &ind : indices)
    {
        rl.push_back(this->at(ind));
    }
    return rl;
}

std::ostream &operator<<(std::ostream &s, const MemoryRegionProperties &m) {
    s << std::hex << std::showbase <<
     "Region Name: " << m.regionName << "\n" <<
     "Parent Region Start: " << m.parentRegionStart << "\n" <<
     "Parent Region Size: " << m.parentRegionSize << "\n" <<
     "Relative Region Start: " << m.relativeRegionStart << "\n" << 
     "Relative Region Size: " << m.relativeRegionSize << "\n";
     return s;
}
