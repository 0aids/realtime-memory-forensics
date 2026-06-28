#pragma once
#include "rmf/maps.hpp"
#include "rmf/snapshots.hpp"
#include <sched.h>
#include <fstream>
#include <format>
#include <cstring>

namespace rmf
{
    struct Process
    {
        const pid_t           pid;

        std::vector<Map>      getMaps() const;

        Snapshot              getSnapshot(const Map& map) const;

        std::vector<Snapshot> getSnapshots(auto&& maps) const;

        // Split a map up into it's active regions, IE regions that are currently in memory.
        std::vector<Map> mapGetActive() const;
    };
} // namespace rmf

namespace rmf
{
    std::vector<Map> Process::getMaps() const
    {
        std::ifstream mapFile =
            std::ifstream(std::format("/proc/{}/maps", pid));
        std::string      line;
        uint32_t         unnamedRegionNumber = 0;
        std::vector<Map> maps;

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
                .perms = Perms::Parse(perms),
            });
        }
        return maps;
    }

    Snapshot Process::getSnapshot(const Map& map) const
    {
        assert(false && "TODO!");
    }

    std::vector<Snapshot> Process::getSnapshots(auto&& maps) const
    {
        assert(false && "TODO!");
    }

    std::vector<Map> Process::mapGetActive() const
    {
        assert(false && "TODO!");
    }
} // namespace rmf
