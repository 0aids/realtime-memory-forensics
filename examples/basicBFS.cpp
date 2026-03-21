#include "logger.hpp"
#include "operations.hpp"
#include "types.hpp"
#include "utils.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <print>
#include <rmf.hpp>
#include <iostream>
#include <sched.h>
#include <chrono>
#include <map>
#include <unordered_set>

using namespace rmf;
using namespace rmf::types;
using namespace std;

using FlatPointerList = std::vector<std::pair<uintptr_t, uintptr_t>>;
using MapifiedSnap =
    std::pair<FlatPointerList, MemoryRegionProperties>;

MapifiedSnap mapifySnap(const MemorySnapshot& snap)
{
    MapifiedSnap pointers;
    pointers.second = snap.getMrp();

    const auto mrp  = snap.getMrp();
    const auto data = snap.getDataSpan();

    ptrdiff_t  head =
        sizeof(uintptr_t) - mrp.TrueAddress() % sizeof(uintptr_t);

    pointers.first.reserve((mrp.relativeRegionSize - head) /
                           sizeof(uintptr_t));

    const uintptr_t* ptr_data =
        reinterpret_cast<const uintptr_t*>(data.data() + head);
    size_t count =
        (mrp.relativeRegionSize - head) / sizeof(uintptr_t);

    for (size_t i = 0; i < count; ++i)
    {
        uintptr_t target = ptr_data[i];
        uintptr_t source =
            head + (i * sizeof(uintptr_t)) + mrp.TrueAddress();
        pointers.first.emplace_back(target, source);
    }

    // Sort the flat array by target address for fast binary searching later
    std::sort(pointers.first.begin(), pointers.first.end(),
              [](const auto& a, const auto& b)
              { return a.first < b.first; });

    return pointers;
}

MemoryRegionPropertiesVec
findSourceRegionsFast(const MapifiedSnap&              mapsnap,
                      const MemoryRegionPropertiesVec& regions,
                      const MrpRestructure&            mrpRestructure)
{
    MemoryRegionPropertiesVec mrpVec;
    const auto&               flat_pointers = mapsnap.first;

    for (auto region : regions)
    {
        region = utils::RestructureMrp(region, mrpRestructure);

        // Binary search the flat vector
        auto bottom = std::lower_bound(
            flat_pointers.begin(), flat_pointers.end(),
            region.TrueAddress(),
            [](const std::pair<uintptr_t, uintptr_t>& element,
               uintptr_t val) { return element.first < val; });

        auto top = std::upper_bound(
            flat_pointers.begin(), flat_pointers.end(),
            region.TrueEnd() - 1,
            [](uintptr_t                              val,
               const std::pair<uintptr_t, uintptr_t>& element)
            { return val < element.first; });

        for (auto it = bottom; it != top; ++it)
        {
            MemoryRegionProperties newMrp = mapsnap.second;
            newMrp.relativeRegionAddress =
                it->second - newMrp.parentRegionAddress;
            newMrp.relativeRegionSize = sizeof(uintptr_t);
            mrpVec.push_back(newMrp);
        }
    }
    return mrpVec;
}
bool operator==(const rmf::types::MemoryRegionProperties& a,
                const rmf::types::MemoryRegionProperties& b)
{
    return a.TrueAddress() == b.TrueAddress() &&
        a.TrueEnd() == b.TrueEnd();
}
bool operator==(const rmf::types::MemoryRegionProperties a,
                const rmf::types::MemoryRegionProperties b)
{
    return a.TrueAddress() == b.TrueAddress() &&
        a.TrueEnd() == b.TrueEnd();
}

int main(int argc, const char** argv)
{
    if (argc != 3)
    {
        cout << "Incorrect arguments! Expecting PID as second "
                "argument AND string to match for as third"
             << endl;
    }
    const pid_t pid         = std::stoul(argv[1]);
    std::string matchString = argv[2];

    rmf::g_logLevel = rmf_Info;

    auto og = rmf::utils::getMapsFromPid(pid)
                  .FilterHasPerms("r")
                  .FilterActiveRegions(pid);
    auto          result = og;
    rmf::Analyzer analyzer(std::thread::hardware_concurrency());
    auto snaps = analyzer.Execute(rmf::types::MemorySnapshot::Make,
                                  result, pid);
    {
        result =
            analyzer.Execute(rmf::op::findString, snaps, matchString)
                .flatten();
        this_thread::sleep_for(1s);
        cout << format("num Results: {}", result.size()) << endl;
    }
    auto   mapsSnaps = analyzer.Execute(mapifySnap, snaps);
    size_t i         = 0;
    while (result.size() > 0)
    {
        result =
            analyzer
                .Execute(findSourceRegionsFast, mapsSnaps, result,
                         rmf::types::MrpRestructure{-8, 16})
                .flatten();
        // Remove duplicats
        cout << format(
                    "num Results after bfs{{{}}} with duplicates: {}",
                    i, result.size())
             << endl;
        std::sort(result.begin(), result.end(),
                  [](const MemoryRegionProperties& a,
                     const MemoryRegionProperties& b)
                  {
                      if (a.TrueAddress() != b.TrueAddress())
                          return a.TrueAddress() < b.TrueAddress();
                      return a.TrueEnd() < b.TrueEnd();
                  });
        result.erase(std::unique(result.begin(), result.end()),
                     result.end());
        cout << format(
                    "num Results after bfs{{{}}} no duplicates: {}",
                    i++, result.size())
             << endl;
    }
}
