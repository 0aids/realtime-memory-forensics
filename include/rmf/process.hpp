#pragma once
#include "rmf/maps.hpp"
#include "rmf/snapshots.hpp"
#include <ranges>
#include <sched.h>
#include <type_traits>

namespace rmf
{
    struct Process;
    struct MapsProcVec;

    struct Process
    {
        const pid_t pid = 0;

        MapsProcVec getMaps() const;

        Snapshot    getSnapshot(const Map& map) const;

        template <std::ranges::random_access_range MapsRange>
            requires std::is_convertible_v<
                std::ranges::range_value_t<MapsRange>, Map>
        std::vector<Snapshot> getSnapshots(const MapsRange& maps) const;

        // Split a map up into it's active regions, IE regions that are currently in memory.
        MapsProcVec mapGetActive(const Map& map) const;
        std::string getPagemapPath() const;

      private:
        std::vector<Map> mapGetActiveImpl(const Map& map,
                                          int        fileDescriptor) const;
    };

    struct MapsProcVec : public std::vector<Map>
    {
        MapsProcVec(const Process& proc, const std::vector<Map>& map);
        Process proc;
        using std::vector<Map>::vector;
        MapsProcVec getActive() const;
    };
} // namespace rmf

namespace rmf
{
    template <std::ranges::random_access_range MapsRange>
        requires std::is_convertible_v<std::ranges::range_value_t<MapsRange>,
                                       Map>
    std::vector<Snapshot> Process::getSnapshots(const MapsRange& maps) const
    {
        std::vector<Snapshot> snapshotsVec;
        snapshotsVec.reserve(maps.size());
        for (const auto& map : maps)
        {
            snapshotsVec.push_back(getSnapshot(map));
        }
        return snapshotsVec;
    }
} // namespace rmf
