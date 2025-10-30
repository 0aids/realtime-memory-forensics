#include "maps.hpp"
#include <fcntl.h>
#include <fstream>
#include <string>
#include "logs.hpp"
#include <algorithm>
#include <cstring>
#include <unistd.h>

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

static constexpr std::string pidToPagemap(pid_t pid)
{
    return "/proc/" + std::to_string(pid) + "/pagemap";
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

RegionPropertiesList RegionPropertiesList::filterRegionsByNotPerms(
    const std::string_view& perms)
{
    Perms permsToMatch = parsePerms(perms);
    RegionPropertiesList rl;
    for (size_t i = 0; i < this->size(); i++) {
        const Perms &p = this->at(i).perms;
        if ((p & permsToMatch) != permsToMatch) 
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

std::ostream &operator<<(std::ostream &s, const Perms &p) {
    if ((bool)(p & Perms::READ))
        s << 'r';
    if ((bool)(p & Perms::WRITE))
        s << 'w';
    if ((bool)(p & Perms::EXECUTE))
        s << 'x';
    if ((bool)(p & Perms::PRIVATE))
        s << 'p';
    if ((bool)(p & Perms::SHARED))
        s << 's';
    return s;
}

std::ostream &operator<<(std::ostream &s, const MemoryRegionProperties &m) {
    s << std::hex << std::showbase <<
     "Region Name: " << m.regionName << "\n" <<
     "Parent Region Start: " << m.parentRegionStart << "\n" <<
     "Parent Region Size: " << m.parentRegionSize << "\n" <<
     "Relative Region Start: " << m.relativeRegionStart << "\n" << 
     "Relative Region Size: " << m.relativeRegionSize << "\n" <<
     "Relative Region Size: " << m.perms << "\n";
     return s;
}

// Returns a large list of PAGE_SIZE regions.
// I'm lazy to consolidate them, plus it sort of serves as a pseudo
// scheduler that makes large regions smaller.
RegionPropertiesList getActiveRegions(const MemoryRegionProperties &mrp) {
    RegionPropertiesList regions;
    long pageSize = sysconf(_SC_PAGE_SIZE);
    const std::string pagemapPath = pidToPagemap(mrp.pid);
    int fd = open(pagemapPath.c_str(), O_RDONLY);
    const uint64_t ACTIVE_BIT = (1ULL << 63);
    for (uintptr_t addr = mrp.getActualRegionStart();
         addr < mrp.getActualRegionStart() + mrp.relativeRegionSize;
         addr += pageSize)
    {
        // Multiply by 8 because each 8 byte chunk represents a page.
        uintptr_t offset = (addr / pageSize) * 8;
        if (lseek(fd, offset, SEEK_SET) == (off_t) -1) {
            Log(Error, "Failed to seek the pagemap!");
            perror("failed to seek pagemap");
            continue;
        }

        uint64_t entry;
        if (read(fd, &entry, 8) != 8) {
            Log(Error, "Failed to READ the pagemap!")
            perror("failed to read pagemap");
            continue;
        }

        if (entry & ACTIVE_BIT) {
            MemoryRegionProperties newMrp = mrp;
            newMrp.relativeRegionSize = pageSize;
            newMrp.relativeRegionStart = addr;
            regions.push_back(newMrp);
        }
    }
    return regions;
}

RegionPropertiesList getActiveRegionsFromRegionPropertiesList(
    const RegionPropertiesList &rpl
)
{
    if (rpl.size() == 0){
        Log(Warning, "Given an empty RegionPropertiesList!!!");
        return {};
    }
    RegionPropertiesList regions;
    long pageSize = sysconf(_SC_PAGE_SIZE);
    const std::string pagemapPath = pidToPagemap(rpl[0].pid);
    int fd = open(pagemapPath.c_str(), O_RDONLY);
    if (fd < 0) {
        Log(Error, "Failed to open the pageMap!!!!");
        return {};
    }
    const uint64_t ACTIVE_BIT = (1ULL << 63);
    for (const auto &mrp:rpl) {
        for (uintptr_t addr = mrp.getActualRegionStart();
             addr < mrp.getActualRegionStart() + mrp.relativeRegionSize;
             addr += pageSize)
        {
            // Multiply by 8 because each 8 byte chunk represents a page.
            uintptr_t offset = (addr / pageSize) * 8;
            if (lseek(fd, offset, SEEK_SET) == (off_t) -1) {
                Log(Error, "Failed to seek the pagemap!");
                perror("failed to seek pagemap");
                continue;
            }

            uint64_t entry;
            if (read(fd, &entry, 8) != 8) {
                Log(Error, "Failed to READ the pagemap!")
                perror("failed to read pagemap");
                continue;
            }

            if (entry & ACTIVE_BIT) {
                MemoryRegionProperties newMrp = mrp;
                newMrp.relativeRegionSize = pageSize;
                newMrp.relativeRegionStart = addr - mrp.parentRegionStart;
                regions.push_back(newMrp);
            }
        }
    }
    close(fd);
    return regions;
}
RegionPropertiesList breakIntoRegionChunks(
    const RegionPropertiesList &rpl, size_t overlap
){
    long pageSize = sysconf(_SC_PAGE_SIZE);
    RegionPropertiesList regions;
    for (const auto &mrp:rpl) {
        for (uintptr_t addr = mrp.getActualRegionStart();
             addr < mrp.getActualRegionStart() + mrp.relativeRegionSize;
             addr += pageSize)
        {
            // Multiply by 8 because each 8 byte chunk represents a page.
            uintptr_t offset = (addr / pageSize) * 8;
            MemoryRegionProperties newMrp = mrp;
            newMrp.relativeRegionSize = pageSize;
            newMrp.relativeRegionStart = addr - mrp.parentRegionStart;
            regions.push_back(newMrp);
        }
    }
    return regions;
}
