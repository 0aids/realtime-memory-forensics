#include "log.hpp"
#include "memory_region.hpp"
#include "memory_map.hpp"
#include <cstring>
#include <cstdlib>
#include <csignal>
#include <iostream>
#include <thread>
#include "region_properties.hpp"
#include "run_test.hpp"
using namespace std;

int main()
{
    pid_t pid = runSampleProcess();
    Log(Message, "Sample process pid: " << pid);
    MemoryMap map(pid);
    auto      mmap         = map.snapshotMaps();
    auto      writeRegions = mmap.getRegionsWithPermissions("rwp");
    assert(writeRegions.size() > 0,
           "There should be readwritable regions");
    auto heapProperties_o = writeRegions.getRegionWithName("[heap]");
    assert(heapProperties_o.has_value(),
           "There should be a region labelled [heap]");

    auto&        heapProperties = heapProperties_o.value();

    MemoryRegion mr(heapProperties, pid);

    assert(mr.snapshot(), "The heap should be snapshottable");

    auto lastSnap = mr.getLastSnapshot();
    auto ptrlikes = lastSnap->findPointerLikes();
    assert(ptrlikes.size() > 0,
           "There should be ptrlikes in memory...");
    size_t total = 0;
    for (size_t i = 0; i < ptrlikes.size(); i++)
    {
        MemoryRegion tempMr(ptrlikes[i], pid);
        tempMr.m_regionProperties.regionSize += 24;
        tempMr.snapshot();
        auto tempLastSnap = tempMr.getLastSnapshot();
        cerr << "Contents: \n" << tempLastSnap->toStr() << endl;
        auto linkers = lastSnap->findLinkedList(
            ptrlikes[i].getActualRegionStart(), 8);
        total += linkers.size();
    }
    cerr << "Number of linkers: " << total << endl;
    assert(total > 0, "There is a linked list in there somewhere...");
}
