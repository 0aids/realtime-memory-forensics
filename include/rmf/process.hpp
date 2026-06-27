#pragma once
#include "rmf/config.hpp"
#include "rmf/maps.hpp"
#include "rmf/snapshots.hpp"
#include <sched.h>
#include <fstream>
#include <format>
#include <cstring>
#include <type_traits>

namespace rmf
{
    struct Process
    {
        const pid_t pid;

        // If you want to add custom allocators, use a "using" statement
        // like:
        //    template <typename T>
        //    using MyVector = std::vector<T, MyAllocator>;
        template <
            template <typename> typename VectorLike = config::DefaultVectorLike>
            requires std::ranges::range<VectorLike<Map>>
        VectorLike<Map> getMaps() const;

        // If you want to add custom allocators, use a "using" statement
        // like:
        //    template <typename T>
        //    using MyVector = std::vector<T, MyAllocator>;
        template <
            template <typename> typename VectorLike = config::DefaultVectorLike>
            requires std::ranges::contiguous_range<VectorLike<uint8_t>>
        Snapshot<VectorLike> getSnapshot(const Map& map) const;

        /* This is true modern c++ */

        // SnapshotsList getSnapshots(RangeWithMaps)
        //
        // Takes in two template parameters -
        // 		* SnapshotVectorLike - Contiguous container for captured memory
        // 		* SnapshotContainerLike - Container for holding snapshots
        // Post requires clauses is to ensure maps is a valid range with maps inside.
        template <template <typename> typename SnapshotVectorLike =
                      config::DefaultVectorLike,
                  template <typename> typename SnapshotContainerLike =
                      SnapshotVectorLike>
        SnapshotContainerLike<Snapshot<SnapshotVectorLike>>
        getSnapshots(auto&& maps) const
            requires std::ranges::contiguous_range<
                         SnapshotVectorLike<uint8_t>> &&
                     std::ranges::range<
                         SnapshotContainerLike<Snapshot<SnapshotVectorLike>>> &&
                     std::ranges::range<std::decay_t<decltype(maps)>> &&
                     std::same_as<std::decay_t<std::ranges::range_value_t<
                                      decltype(maps)>>,
                                  Map>;

        // Split a map up into it's active regions, IE regions that are currently in memory.
        template <template <typename> typename MapsContainerLike =
                      config::DefaultVectorLike>
        MapsContainerLike<Map> mapGetActive() const;
    };
} // namespace rmf

namespace rmf
{
    template <template <typename> typename VectorLike>
        requires std::ranges::range<VectorLike<Map>>
    VectorLike<Map> Process::getMaps() const
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

    template <template <typename> typename VectorLike>
        requires std::ranges::contiguous_range<VectorLike<uint8_t>>
    Snapshot<VectorLike> Process::getSnapshot(const Map& map) const
    {
        assert(false && "TODO!");
    }

    template <template <typename> typename SnapshotVectorLike,
              template <typename> typename SnapshotContainerLike>
    SnapshotContainerLike<Snapshot<SnapshotVectorLike>>
    Process::getSnapshots(auto&& maps) const
        requires std::ranges::contiguous_range<SnapshotVectorLike<uint8_t>> &&
                 std::ranges::range<
                     SnapshotContainerLike<Snapshot<SnapshotVectorLike>>> &&
                 std::ranges::range<std::decay_t<decltype(maps)>> &&
                 std::same_as<
                     std::decay_t<std::ranges::range_value_t<decltype(maps)>>,
                     Map>
    {
        assert(false && "TODO!");
    }

    template <template <typename> typename MapsContainerLike>
    MapsContainerLike<Map> Process::mapGetActive() const
    {
        assert(false && "TODO!");
    }
} // namespace rmf
