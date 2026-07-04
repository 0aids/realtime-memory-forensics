#include "rmf/process.hpp"
#include <stdexcept>
extern "C"
{
#include <fcntl.h>
#include <sys/uio.h>
}
#include <fstream>
#include <format>
#include <cstring>

namespace rmf
{
    MapsProcVec::MapsProcVec(const Process&          proc,
                             const std::vector<Map>& maps) :
        std::vector<Map>(maps), proc(proc)
    {
    }

    MapsProcVec MapsProcVec::getActive() const
    {
        auto mv = MapsProcVec(proc, *this);
        for (const auto& map : *this)
        {
            auto active = proc.mapGetActive(map);
            std::move(active.begin(), active.end(), std::back_inserter(mv));
        }
        return mv;
    }

    MapsProcVec Process::getMaps() const
    {
        std::ifstream mapFile =
            std::ifstream(std::format("/proc/{}/maps", pid));
        std::string line;
        uint32_t    unnamedRegionNumber = 0;
        MapsProcVec maps                = MapsProcVec(*this, {});

        while (std::getline(mapFile, line))
        {
            uintptr_t   startAddr, endAddr;
            std::string perms;
            perms.resize(4, '-');
            std::string name;
            name.resize(1024, 0);
            sscanf(line.c_str(), "%lx-%lx %c%c%c%c %*s %*s %*s %[^\n]",
                   &startAddr, &endAddr, &perms[0], &perms[1], &perms[2],
                   &perms[3], name.data());
            name.resize(std::strlen(name.c_str()));

            if (name.size() == 0)
                name = std::format("AnonymousRegion-{}", unnamedRegionNumber++);
            maps.push_back(Map{
                .name  = std::make_shared<const std::string>(std::move(name)),
                .pAddr = startAddr,
                .pSize = endAddr - startAddr,
                .rAddr = 0,
                .rSize = static_cast<ptrdiff_t>(endAddr - startAddr),
                .perms = Perms_Parse(perms),
            });
        }
        return maps;
    }

    Snapshot Process::getSnapshot(const Map& map) const
    {
        constexpr ptrdiff_t chunkSize = 1 << 24;

        struct iovec        localIovec[1];
        struct iovec        sourceIovec[1];
        Snapshot            snap = {};
        snap.resize(map.rSize);
        intptr_t totalBytesRead = 0;
        while (totalBytesRead < static_cast<intptr_t>(map.rSize))
        {
            uintptr_t bytesToRead = (map.rSize - totalBytesRead > chunkSize) ?
                                        chunkSize :
                                        map.rSize - totalBytesRead;

            sourceIovec[0].iov_base = (void*)(map.tbegin() + totalBytesRead);
            sourceIovec[0].iov_len  = bytesToRead;

            localIovec[0].iov_base = snap.data() + totalBytesRead;
            localIovec[0].iov_len  = bytesToRead;

            ssize_t nread =
                process_vm_readv(pid, localIovec, 1, sourceIovec, 1, 0);

            if (nread <= 0)
            {
                if (nread == -1 && totalBytesRead > 0)
                {
                    snap.clear();
                    perror("process_vm_readv");
                    return snap;
                }
                perror("process_vm_readv");
                snap.resize(totalBytesRead);
                return snap;
            }
            totalBytesRead += nread;
        }
        return snap;
    }
    std::string Process::getPagemapPath() const
    {
        return std::format("/proc/{}/pagemap", pid);
    }

    MapsProcVec Process::mapGetActive(const Map& map) const
    {
        const std::string pagemapPath = getPagemapPath();
        int               fd          = open(pagemapPath.c_str(), O_RDONLY);
        if (fd < 0)
        {
            throw std::runtime_error("Failed to open the pagemap!");
        }
        auto activeRegions = MapsProcVec(*this, mapGetActiveImpl(map, fd));
        close(fd);
        return activeRegions;
    }

    std::vector<Map> Process::mapGetActiveImpl(const Map& map,
                                               int        fileDescriptor) const
    {
        long                      pageSize   = sysconf(_SC_PAGE_SIZE);
        static constexpr uint64_t ACTIVE_BIT = (1ULL << 63);
        std::vector<Map>          regions;
        for (uintptr_t addr = map.tbegin(); addr < map.tend(); addr += pageSize)
        {
            // Multiply by 8 because each 8 byte chunk represents a page.
            uintptr_t offset = (addr / pageSize) * 8;
            if (lseek(fileDescriptor, offset, SEEK_SET) == (off_t)-1)
            {
                perror("failed to seek pagemap");
                continue;
            }

            uint64_t entry;
            ssize_t  readResult = read(fileDescriptor, &entry, 8);
            if (readResult == (ssize_t)-1)
            {
                perror("failed to read pagemap");
                continue;
            }

            else if (readResult < (ssize_t)sizeof(entry))
            {
                continue;
            }

            if (entry & ACTIVE_BIT)
            {
                if (regions.size() > 0 &&
                    regions.back().rend() == addr - map.pAddr)
                {
                    regions.back().rSize += pageSize;
                }
                else
                {
                    Map newMap   = map;
                    newMap.rSize = pageSize;
                    newMap.rAddr = addr - map.pAddr;
                    regions.push_back(newMap);
                }
            }
        }
        return regions;
    }
} // namespace rmf
