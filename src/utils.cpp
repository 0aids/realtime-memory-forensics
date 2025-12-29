#include "utils.hpp"
#include "types.hpp"
#include <fstream>
#include <cstring>

using namespace magic_enum::bitwise_operators; // out-of-the-box bitwise operators for enums.

namespace rmf::utils {
rmf::types::Perms ParsePerms(const std::string_view& perms) {
    rmf::types::Perms ps{};
    if (perms.contains('r')) {
        ps |= rmf::types::Perms::Read;
    }
    if (perms.contains('w')) {
        ps |= rmf::types::Perms::Write;
    }
    if (perms.contains('x')) {
        ps |= rmf::types::Perms::Execute;
    }
    if (perms.contains('s')) {
        ps |= rmf::types::Perms::Shared;
    }
    return ps;
}

rmf::types::MemoryRegionPropertiesVec
FilterMaxSize(const rmf::types::MemoryRegionPropertiesVec& other,
              const uintptr_t                              maxSize)
{
    rmf::types::MemoryRegionPropertiesVec rl;
    for (size_t i = 0; i < other.size(); i++)
    {
        if (other.at(i).relativeRegionSize <= maxSize)
        {
            rl.push_back(other.at(i));
        }
    }

    return rl;
}

rmf::types::MemoryRegionPropertiesVec
FilterMinSize(const rmf::types::MemoryRegionPropertiesVec& other,
              const uintptr_t                              minSize)
{
    rmf::types::MemoryRegionPropertiesVec rl;
    for (size_t i = 0; i < other.size(); i++)
    {
        if (other.at(i).relativeRegionSize >= minSize)
        {
            rl.push_back(other.at(i));
        }
    }

    return rl;
}

rmf::types::MemoryRegionPropertiesVec
FilterName(const rmf::types::MemoryRegionPropertiesVec& other,
           const std::string_view&                      string)
{
    rmf::types::MemoryRegionPropertiesVec rl;
    for (size_t i = 0; i < other.size(); i++)
    {
        if (other.at(i).regionName == string)
        {
            rl.push_back(other.at(i));
        }
    }

    return rl;
}

rmf::types::MemoryRegionPropertiesVec
FilterSubName(const rmf::types::MemoryRegionPropertiesVec& other,
              const std::string_view&                      string)
{
    rmf::types::MemoryRegionPropertiesVec rl;
    for (size_t i = 0; i < other.size(); i++)
    {
        if (other.at(i).regionName.contains(string))
        {
            rl.push_back(other.at(i));
        }
    }

    return rl;
}

rmf::types::MemoryRegionPropertiesVec
FilterPerms(const rmf::types::MemoryRegionPropertiesVec& other,
            const std::string_view&                      perms)
{
    rmf::types::Perms permsToMatch = rmf::utils::ParsePerms(perms);
    rmf::types::MemoryRegionPropertiesVec rl;
    for (size_t i = 0; i < other.size(); i++)
    {
        const rmf::types::Perms& p = other.at(i).perms;
        if (p == permsToMatch)
        {
            rl.push_back(other.at(i));
        }
    }
    return rl;
}

rmf::types::MemoryRegionPropertiesVec
FilterHasPerms(const rmf::types::MemoryRegionPropertiesVec& other,
               const std::string_view&                      perms)
{
    rmf::types::Perms permsToMatch = rmf::utils::ParsePerms(perms);
    rmf::types::MemoryRegionPropertiesVec rl;
    for (size_t i = 0; i < other.size(); i++)
    {
        const rmf::types::Perms& p = other.at(i).perms;
        if ((p & permsToMatch) == permsToMatch)
        {
            rl.push_back(other.at(i));
        }
    }
    return rl;
}

rmf::types::MemoryRegionPropertiesVec
FilterNotPerms(const rmf::types::MemoryRegionPropertiesVec& other,
               const std::string_view&                      perms)
{
    rmf::types::Perms permsToMatch = rmf::utils::ParsePerms(perms);
    rmf::types::MemoryRegionPropertiesVec rl;
    for (size_t i = 0; i < other.size(); i++)
    {
        const rmf::types::Perms& p = other.at(i).perms;
        if ((p & permsToMatch) != permsToMatch)
        {
            rl.push_back(other.at(i));
        }
    }
    return rl;
}
rmf::types::MemoryRegionPropertiesVec ParseMaps(const std::string&& fullPath, pid_t pid) {
    std::ifstream        memoryMapFile(fullPath);
    std::string          line;
    int                  unnamedRegionNumber = 1;

    rmf::types::MemoryRegionPropertiesVec regionProperties;

    while (std::getline(memoryMapFile, line))
    {
        uintptr_t startAddr, endAddr;
        std::string perms;
        perms.resize(4, '-');
        std::string name;
        name.resize(1024, 0);
        sscanf(line.c_str(), "%lx-%lx %c%c%c%c %*s %*s %*s %[^\n]",
               &startAddr, &endAddr, &perms[0], &perms[1], &perms[2], &perms[3],
               name.data());
        name.resize(strlen(name.c_str()));

        if (name.size() == 0)
        {
            name = "UnnamedRegion-" +
                std::to_string(unnamedRegionNumber++);
        }

        rmf::types::MemoryRegionProperties m = {
            startAddr,
            endAddr - startAddr,
            0,
            endAddr - startAddr,
            name,
            rmf::utils::ParsePerms(perms),
            pid,
        };
        rmf_Log(rmf_Debug, "Found region: " << m.toString());
        regionProperties.push_back(std::move(m));
    }
    return regionProperties;

}

std::string PidToMapsString(const pid_t pid) 
{
    return "/proc/" + std::to_string(pid) + "/maps";
}
}
