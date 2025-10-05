// Testing retrieving region data.
#include "log.hpp"
#include "memory_region.hpp"
#include "memory_map.hpp"
#include <cstdlib>
#include <csignal>
#include <iostream>
#include <thread>
#include "run_test.hpp"
using namespace std;
int main()
{
    pid_t pid = runSampleProcess();
    Log(Message, "Sample process pid: " << pid);

    MemoryMap map(pid);
    auto      mmap = map.snapshotMaps();

    // Most if not all regions are private
    auto writeRegions = mmap.getRegionsWithPermissions("rwp");
    Log(Message,
        "Number of read writable regions: " << writeRegions.size());
    for (const auto& region : writeRegions)
    {
        Log(Message, "ReadWritable Region:\n" << region.toConstStr());
    }
    assert(writeRegions.size() > 0, "No regions found!!!");

    // Find the region that is changing.
    int stringLikeCount = 0;
    Log(Debug, "Finding regions that are changing");
    for (const auto& regionProperties : writeRegions)
    {
        MemoryRegion mem(regionProperties, pid);
        mem.snapshot();
        auto snap = mem.getLastSnapshot();

        auto properties = snap->findStringLikeRegions(10);
        Log(Debug,
            "Number of stringlike regions: " << properties.size());
        if (properties.size() > 0)
        {
            stringLikeCount++;
            for (const auto& property : properties)
            {
                MemoryRegion stringLikeRegion(property, pid);
                stringLikeRegion.snapshot();
                // Log(Message,
                //     "StringLikeRegion's properties: \n"
                //         << properties.front().toConstStr());
                Log(Message,
                    "Region Name: " << property.parentRegionName);
                Log(Message,
                    "Sample stringlike region: "
                        << stringLikeRegion.getLastSnapshot()
                               ->toStr());
            }
        }
    }
    assert(stringLikeCount > 0,
           "Did not find any stringlike regions");

    Log(Debug, "A region that was stringlike was found");
    kill(pid, SIGTERM);
};
