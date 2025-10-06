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
    auto results  = lastSnap->findDoubleLike(140, 160);
    Log(Message, "Results size: " << results.size());
    assert(results.size() > 0,
           "There should be a double in the memory");
    double value;
    memcpy(&value,
           lastSnap->data() + results.back().relativeRegionStart,
           sizeof(double));
    Log(Message, "The double's value: " << value)
}
