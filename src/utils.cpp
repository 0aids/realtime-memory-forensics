#include "utils.hpp"
#include "logger.hpp"
#include "types.hpp"
#include <fstream>
#include <cstring>

using namespace magic_enum::bitwise_operators; // out-of-the-box bitwise operators for enums.

namespace rmf::utils {
rmf::types::Perms ParsePerms(const std::string_view& perms) {
    rmf::types::Perms ps = rmf::types::Perms::None;
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
        if (*(other.at(i).regionName_sp) == string)
        {
            rl.push_back(other.at(i));
        }
    }

    return rl;
}

rmf::types::MemoryRegionPropertiesVec
FilterContainsName(const rmf::types::MemoryRegionPropertiesVec& other,
              const std::string_view&                      string)
{
    rmf::types::MemoryRegionPropertiesVec rl;
    for (size_t i = 0; i < other.size(); i++)
    {
        if (other.at(i).regionName_sp->contains(string))
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
rmf::types::MemoryRegionPropertiesVec ParseMaps(const std::string fullPath, pid_t pid) {
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
            std::make_shared<const std::string>(name),
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

rmf::types::MemoryRegionPropertiesVec
BreakIntoChunks(const rmf::types::MemoryRegionProperties& other,
                const uintptr_t chunkSize, uintptr_t ovelapSize)
{
    rmf::types::MemoryRegionPropertiesVec res;
    res.reserve(other.relativeRegionSize / chunkSize + 1);
    uintptr_t ptrHead = other.relativeRegionAddress;
    const uintptr_t end = other.relativeEnd();

    while (ptrHead < end) {
        const uintptr_t actualChunkSize = (end - ptrHead > chunkSize) ? chunkSize : end - ptrHead;
        res.push_back(other);
        res.back().relativeRegionSize = actualChunkSize;
        res.back().relativeRegionAddress = ptrHead;
        ptrHead += actualChunkSize;
        if (ptrHead >= end) break;
        ptrHead -= ovelapSize;
    }
    rmf_Log(rmf_Debug, "Broken into: " << res.size() << " chunks");
    return res;
}

rmf::types::MemoryRegionPropertiesVec BreakIntoChunks(
    const rmf::types::MemoryRegionPropertiesVec& other,
    uintptr_t chunkSize, uintptr_t ovelapSize)
{
    rmf::types::MemoryRegionPropertiesVec res;
    uintptr_t overallSize = 0;
    for (const auto& mrp:other) {
        overallSize += mrp.relativeRegionSize;
    }

    res.reserve(overallSize / chunkSize + 1);

    for (const auto& mrp:other) {
        uintptr_t ptrHead = mrp.relativeRegionAddress;
        const uintptr_t end = mrp.relativeEnd();

        while (ptrHead < end) {
            const uintptr_t actualChunkSize = (end - ptrHead > chunkSize) ? chunkSize : end - ptrHead;
            res.push_back(mrp);
            res.back().relativeRegionSize = actualChunkSize;
            res.back().relativeRegionAddress = ptrHead;
            ptrHead += actualChunkSize;
            if (ptrHead >= end) break;
            ptrHead -= ovelapSize;
        }
    }
    rmf_Log(rmf_Debug, "Total size: " << std::hex << std::showbase << overallSize);
    rmf_Log(rmf_Debug, "Broken into: " << res.size() << " chunks");
    return res;
}

rmf::types::MemoryRegionPropertiesVec
CompressNestedMrpVec(const std::vector<types::MemoryRegionPropertiesVec> &mrpvecVec)
{
    rmf::types::MemoryRegionPropertiesVec res;
    size_t total = 0;
    for (auto& mrpVec:mrpvecVec) {
        total += mrpVec.size();
    }
    res.reserve(total);
    for (auto& mrpVec:mrpvecVec) {
        for (auto& mrp: mrpVec)
        {
            res.push_back(mrp);
        }
    }
    return res;
}

}
