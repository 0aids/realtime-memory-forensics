#pragma once
#include "rmf/maps.hpp"
#include <sched.h>
#include <fstream>
#include <format>
#include <cstring>

namespace rmf
{
    struct Process
    {
        pid_t pid;
        // Any other stuff relating to the process goes here.
        template <template <typename...> typename VectorLike>
            requires(requires(VectorLike<Map> v) { v.push_back(Map{}); }) &&
                    std::ranges::range<VectorLike<Map>>
        VectorLike<Map> getMaps() const
        {
            std::ifstream mapFile =
                std::ifstream(std::format("/proc/{}/maps", pid));
            std::string     line;
            uint32_t        unnamedRegionNumber = 0;
            VectorLike<Map> maps;

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
                    name = std::format("AnonymousRegion-{}",
                                       unnamedRegionNumber++);
                maps.push_back(Map{
                    .name =
                        std::make_shared<const std::string>(std::move(name)),
                    .pAddr = startAddr,
                    .pSize = endAddr - startAddr,
                    .rAddr = 0,
                    .rSize = static_cast<ptrdiff_t>(endAddr - startAddr),
                    .perms = Perms::Parse(perms),
                });
            }
            return maps;
        }
    };
}
