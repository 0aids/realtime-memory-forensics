// Testing retrieving region data.
#include "log.hpp"
#include "memory_region.hpp"
#include "memory_map.hpp"
#include <cstdlib>
#include <iostream>
#include <thread>
#include "run_test.hpp"
using namespace std;
int main() {
    pid_t pid = runSampleProcess();
    Log(Message, "Sample process pid: " << pid);

    MemoryMap map(pid);
    map.readMaps();

    // Most if not all regions are private
    auto writeRegions = map.getRegionsWithPermissions("rwp");
    Log(Message,
        "Number of read writable regions: " << writeRegions.size());
    for (const auto& region : writeRegions) {
        Log(Message, "ReadWritable Region:\n" << region.toConstStr());
    }
    assert(writeRegions.size() > 0, "No regions found!!!");
    auto         firstRegion = writeRegions[0];

    MemoryRegion region(firstRegion, pid);
    region.snapshot();
    Log(Message, "----------------------------");
    Log(Message,
        "First region properties: \n"
            << firstRegion.toConstStr());
    Log(Message,
        "First region contents: \n"
            << region.getLastSnapshot().toStr());

    // The heap
    MemoryRegion anotherRegion(writeRegions[1], pid);
    anotherRegion.snapshot();
    Log(Debug,
        "Heap region: \n"
            << anotherRegion.getLastSnapshot().toStr());
    auto        snap     = anotherRegion.getLastSnapshot();
    std::string sequence = "Lorem ipsum";
    auto it = std::search(snap.begin(), snap.end(), sequence.begin(),
                          sequence.end());

    this_thread::sleep_for(50ms);
    assert(it != snap.end(), "No Lorem ipsum found!!!");
    anotherRegion.snapshot();
    auto snap2 = anotherRegion.getLastSnapshot();

    auto changing = snap.findChangedRegions(snap2, 8);
    Log(Message, "Number of changing regions:" << changing.size());
    auto unchanging = snap.findUnchangedRegions(snap2, 8);
    Log(Message,
        "Number of unchanging regions:" << unchanging.size());

    assert(
        unchanging.size() == 1,
        "Non-changing region should be a contiguous single output");

    // Find the region that is changing.
    Log(Debug, "Finding a regions that are changing");
    for (const auto& regionProperties : writeRegions) {
        MemoryRegion region(regionProperties, pid);
        region.snapshot();
        auto snapA = region.getLastSnapshot();
        this_thread::sleep_for(5ms);
        region.snapshot();
        auto snapB = region.getLastSnapshot();

        auto changing = snapB.findChangedRegions(snapA, 8);
        Log(Debug, "Number of changing regions: " << changing.size());
        if (changing.size() > 0) {
            Log(Message,
                "Found changing region: \n"
                    << changing[0].toConstStr());
            goto foundThing;
        }
    }
    assert(false, "Did not find any changing regions");

foundThing:
    Log(Debug, "A region that was changing was found");
};
