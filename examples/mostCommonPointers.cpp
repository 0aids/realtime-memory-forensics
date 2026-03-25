
#include "logger.hpp"
#include "memory_graph.hpp"
#include "operations.hpp"
#include "types.hpp"
#include "utils.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <print>
#include <ranges>
#include <rmf.hpp>
#include <iostream>
#include <sched.h>
#include <chrono>
#include <map>
#include <thread>
#include <unordered_set>

using namespace rmf;
using namespace rmf::types;
using namespace std;

struct MapifiedSnap
{
    unordered_map<uintptr_t, size_t> targetValueCountsMap;
    MemoryRegionProperties           sourceMrp;
};

MapifiedSnap mapifySnap(const MemorySnapshot& snap)
{
    MapifiedSnap pointers;
    pointers.sourceMrp = snap.getMrp();

    const auto mrp  = snap.getMrp();
    const auto data = snap.getDataSpan();

    ptrdiff_t  head =
        sizeof(uintptr_t) - mrp.TrueAddress() % sizeof(uintptr_t);

    const uintptr_t* ptr_data =
        reinterpret_cast<const uintptr_t*>(data.data() + head);
    size_t count =
        (mrp.relativeRegionSize - head) / sizeof(uintptr_t);

    for (size_t i = 0; i < count; ++i)
    {
        uintptr_t target = ptr_data[i];
        if (pointers.targetValueCountsMap.contains(target))
        {
            pointers.targetValueCountsMap[target]++;
        }
        else
            pointers.targetValueCountsMap[target] = 1;
    }

    // Sort the flat array by target address for fast binary searching later
    return pointers;
}
using CountMap = std::unordered_map<uintptr_t, size_t>;

// Merges a chunk (vector) of maps into a single map.
CountMap mergeMapChunk(std::vector<CountMap> chunk)
{
    if (chunk.empty())
        return {};

    // We accumulate everything into the first map to save allocations
    CountMap result = std::move(chunk[0]);

    for (size_t i = 1; i < chunk.size(); ++i)
    {
        for (const auto& [target, count] : chunk[i])
        {
            result[target] += count;
        }
    }
    return result;
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
    rmf::Analyzer analyzer(std::thread::hardware_concurrency());
    {
        auto og = rmf::utils::getMapsFromPid(pid)
                      .FilterHasPerms("r")
                      .FilterActiveRegions(pid);
        auto normal =
            rmf::utils::getMapsFromPid(pid).FilterHasPerms("r");
        auto snaps = analyzer.Execute(
            rmf::types::MemorySnapshot::Make, og, pid);
        auto mapsSnaps = analyzer.Execute(mapifySnap, snaps);
        if (mapsSnaps.empty())
        {
            println("Empty mapsSnaps!?");
            return 1;
        }

        // Collate the most common ones.

        auto   map1iter = mapsSnaps.begin();
        auto   map1     = map1iter->targetValueCountsMap;
        size_t ind      = 0;
        for (auto mapIter = map1iter + 1; mapIter != mapsSnaps.end();
             mapIter++)
        {
            for (const auto& [target, count] :
                 mapIter->targetValueCountsMap)
            {
                if (!normal.GetRegionContainingAddress(target)
                         .has_value())
                    continue;

                if (map1.contains(target))
                {
                    map1[target] += count;
                }
                else
                    map1[target] = count;
            }
            ind++;
            if (ind % 10000 == 0)
                println("num processed: {} / {}", ind,
                        mapsSnaps.size());
        }

        // ... (Proceed with your sorting and printing logic using map1)
        // print out all of the common values, sorted by frequency.
        // Create a vector to hold the filtered pairs for sorting
        std::vector<std::pair<uintptr_t, size_t>> sorted_counts;

        // Filter out target == 0 and count < 10
        for (const auto& [target, count] : map1)
        {
            if (target != 0 && count >= 500)
            {
                sorted_counts.emplace_back(target, count);
            }
        }

        // Sort by frequency (count) in descending order
        std::sort(sorted_counts.begin(), sorted_counts.end(),
                  [](const auto& a, const auto& b)
                  { return a.second > b.second; });

        // Print out all of the common values, sorted by frequency.
        println("=== Top Targets by Frequency ===");
        size_t i = 0;
        for (const auto& [target, count] : sorted_counts)
        {
            // Formatting the target as hex for easier reading
            println("Target: 0x{:016x} | Count: {}", target, count);
            if (i++ > 1000)
                break;
        }
        println("================================");
        println("========= top 10 regions =========");
        for (size_t i = 0; i < 10; i++)
        {
            println(
                "== [{}] Target: 0x{:016x} | Count: {} == ", i + 1,
                sorted_counts[i].first, sorted_counts[i].second);
            println("{}\n",
                    normal
                        .GetRegionContainingAddress(
                            sorted_counts[i].first)
                        .value()
                        .toString());
        }
    }
}
