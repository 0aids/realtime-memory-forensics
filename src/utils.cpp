#include "utils.hpp"
#include "logger.hpp"
#include "types.hpp"
#include <cstddef>
#include <fstream>
#include <cstring>
#include <fcntl.h>

using namespace magic_enum::
    bitwise_operators; // out-of-the-box bitwise operators for enums.

namespace rmf::utils
{
    rmf::types::Perms ParsePerms(const std::string_view& perms)
    {
        rmf::types::Perms ps = rmf::types::Perms::None;
        if (perms.contains('r'))
        {
            ps |= rmf::types::Perms::Read;
        }
        if (perms.contains('w'))
        {
            ps |= rmf::types::Perms::Write;
        }
        if (perms.contains('x'))
        {
            ps |= rmf::types::Perms::Execute;
        }
        if (perms.contains('s'))
        {
            ps |= rmf::types::Perms::Shared;
        }
        return ps;
    }

    rmf::types::MemoryRegionPropertiesVec
    FilterMaxSize(const rmf::types::MemoryRegionPropertiesVec& other,
                  const uintptr_t maxSize)
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
                  const uintptr_t minSize)
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

    rmf::types::MemoryRegionPropertiesVec FilterContainsName(
        const rmf::types::MemoryRegionPropertiesVec& other,
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

    rmf::types::MemoryRegionPropertiesVec FilterExactPerms(
        const rmf::types::MemoryRegionPropertiesVec& other,
        const std::string_view&                      perms)
    {
        rmf::types::Perms permsToMatch =
            rmf::utils::ParsePerms(perms);
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
        rmf::types::Perms permsToMatch =
            rmf::utils::ParsePerms(perms);
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
        rmf::types::Perms permsToMatch =
            rmf::utils::ParsePerms(perms);
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
    rmf::types::MemoryRegionPropertiesVec
    ParseMaps(const std::string fullPath)
    {
        std::ifstream                         memoryMapFile(fullPath);
        std::string                           line;
        int                                   unnamedRegionNumber = 1;

        rmf::types::MemoryRegionPropertiesVec regionProperties;

        while (std::getline(memoryMapFile, line))
        {
            uintptr_t   startAddr, endAddr;
            std::string perms;
            perms.resize(4, '-');
            std::string name;
            name.resize(1024, 0);
            sscanf(line.c_str(),
                   "%lx-%lx %c%c%c%c %*s %*s %*s %[^\n]", &startAddr,
                   &endAddr, &perms[0], &perms[1], &perms[2],
                   &perms[3], name.data());
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
        uintptr_t       ptrHead = other.relativeRegionAddress;
        const uintptr_t end     = other.relativeEnd();

        while (ptrHead < end)
        {
            const uintptr_t actualChunkSize =
                (end - ptrHead > chunkSize) ? chunkSize :
                                              end - ptrHead;
            res.push_back(other);
            res.back().relativeRegionSize    = actualChunkSize;
            res.back().relativeRegionAddress = ptrHead;
            ptrHead += actualChunkSize;
            if (ptrHead >= end)
                break;
            ptrHead -= ovelapSize;
        }
        rmf_Log(rmf_Debug,
                "Broken into: " << res.size() << " chunks");
        return res;
    }

    rmf::types::MemoryRegionPropertiesVec BreakIntoChunks(
        const rmf::types::MemoryRegionPropertiesVec& other,
        uintptr_t chunkSize, uintptr_t ovelapSize)
    {
        rmf::types::MemoryRegionPropertiesVec res;
        uintptr_t                             overallSize = 0;
        for (const auto& mrp : other)
        {
            overallSize += mrp.relativeRegionSize;
        }

        res.reserve(overallSize / chunkSize + 1);

        for (const auto& mrp : other)
        {
            uintptr_t       ptrHead = mrp.relativeRegionAddress;
            const uintptr_t end     = mrp.relativeEnd();

            while (ptrHead < end)
            {
                const uintptr_t actualChunkSize =
                    (end - ptrHead > chunkSize) ? chunkSize :
                                                  end - ptrHead;
                res.push_back(mrp);
                res.back().relativeRegionSize    = actualChunkSize;
                res.back().relativeRegionAddress = ptrHead;
                ptrHead += actualChunkSize;
                if (ptrHead >= end)
                    break;
                ptrHead -= ovelapSize;
            }
        }
        rmf_Log(rmf_Debug,
                "Total size: " << std::hex << std::showbase
                               << overallSize);
        rmf_Log(rmf_Debug,
                "Broken into: " << res.size() << " chunks");
        return res;
    }

    rmf::types::MemoryRegionPropertiesVec CompressNestedMrpVec(
        const std::vector<types::MemoryRegionPropertiesVec>&
            mrpvecVec)
    {
        rmf::types::MemoryRegionPropertiesVec res;
        size_t                                total = 0;
        for (auto& mrpVec : mrpvecVec)
        {
            total += mrpVec.size();
        }
        res.reserve(total);
        for (auto& mrpVec : mrpvecVec)
        {
            for (auto& mrp : mrpVec)
            {
                res.push_back(mrp);
            }
        }
        return res;
    }

    rmf::types::MemoryRegionPropertiesVec FilterActiveRegions(
        const types::MemoryRegionPropertiesVec& mrpVec, pid_t pid)
    {
        if (mrpVec.size() == 0)
        {
            rmf_Log(
                rmf_Warning,
                "Given an empty types::MemoryRegionPropertiesVec!!!");
            return {};
        }
        types::MemoryRegionPropertiesVec regions;
        long              pageSize = sysconf(_SC_PAGE_SIZE);
        const std::string pagemapPath =
            "/proc/" + std::to_string(pid) + "/pagemap";
        int fd = open(pagemapPath.c_str(), O_RDONLY);
        if (fd < 0)
        {
            rmf_Log(rmf_Error, "Failed to open the pageMap!!!!");
            return {};
        }
        const uint64_t ACTIVE_BIT = (1ULL << 63);
        for (const auto& mrp : mrpVec)
        {
            for (uintptr_t addr = mrp.TrueAddress();
                 addr < mrp.TrueAddress() + mrp.relativeRegionSize;
                 addr += pageSize)
            {
                // Multiply by 8 because each 8 byte chunk represents a page.
                uintptr_t offset = (addr / pageSize) * 8;
                if (lseek(fd, offset, SEEK_SET) == (off_t)-1)
                {
                    rmf_Log(rmf_Error, "Failed to seek the pagemap!");
                    perror("failed to seek pagemap");
                    continue;
                }

                uint64_t entry;
                if (read(fd, &entry, 8) != 8)
                {
                    rmf_Log(rmf_Error, "Failed to READ the pagemap!")
                        perror("failed to read pagemap");
                    continue;
                }

                if (entry & ACTIVE_BIT)
                {
                    types::MemoryRegionProperties newMrp = mrp;
                    newMrp.relativeRegionSize            = pageSize;
                    newMrp.relativeRegionAddress =
                        addr - mrp.parentRegionAddress;
                    regions.push_back(newMrp);
                }
            }
        }
        close(fd);
        return regions;
    }

    rmf::types::MemoryRegionPropertiesVec getMapsFromPid(pid_t pid)
    {
        return ParseMaps(PidToMapsString(pid));
    }
    types::MemoryRegionProperties
    RestructureMrp(const types::MemoryRegionProperties& mrp,
                   const types::MrpRestructure&         restructure)
    {
        auto newmrp = mrp;
        newmrp.relativeRegionAddress += restructure.offset;
        newmrp.relativeRegionSize += restructure.sizeDelta;
        if (newmrp.relativeRegionAddress < 0)
            newmrp.relativeRegionAddress = 0;
        else if (newmrp.relativeRegionAddress >
                 (ptrdiff_t)newmrp.parentRegionSize)
            newmrp.relativeRegionAddress =
                newmrp.parentRegionSize - 1;

        if (newmrp.relativeEnd() > newmrp.parentRegionSize)
            newmrp.relativeRegionSize =
                newmrp.parentRegionSize - newmrp.relativeEnd();
        else if (newmrp.relativeRegionSize < 0)
            newmrp.relativeRegionSize = 1;

        return newmrp;
    }
}
