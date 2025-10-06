
// Testing retrieving region data.
#include "log.hpp"
#include "memory_region.hpp"
#include "memory_map.hpp"
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
    auto      mmap = map.snapshotMaps();

    for (size_t i = 0; i < mmap.size(); i++)
    {
        Log(Debug, mmap[i].toConstStr());
        Log(Debug, "");
    }

    // Most if not all regions are private
    auto writeRegions = mmap.getRegionsWithPermissions("rwp");
    Log(Message,
        "Number of read writable regions: " << writeRegions.size());
    assert(writeRegions.size() > 0, "No regions found!!!");

    auto pr = writeRegions.getRegionWithName("[heap]");
    assert((pr.has_value()),
           "There should be a region called '[heap]'");

    MemoryRegion mr(pr.value(), pid);
    mr.snapshot();
    auto lastSnap = mr.getLastSnapshot();
    auto loc      = lastSnap->findOf("small string");
    assert(loc.size() > 0, "There should be a small string!");
    Log(Message,
        "Found small string at address: "
            << std::hex << std::showbase
            << loc.back().getActualRegionStart());
    Log(Message,
        "Looking for a memory address that points to the ^^^^^^...");
    auto ptrs =
        lastSnap->findPointers(loc.back().getActualRegionStart());
    assert(
        ptrs.size() > 0,
        "There definitely should be a pointer pointing to the 'small "
        "string'...");
    Log(Message,
        "Pointer pointing to 'small string' was found. Address: "
            << std::hex << std::showbase
            << ptrs.back().getActualRegionStart());

    Log(Debug, "Looking for the value in the stack. ");

    auto stackProperties = mmap.getRegionWithName("[stack]");
    assert(stackProperties.has_value(),
           "There should be a region named '[stack]'");

    MemoryRegion stackRegion(stackProperties.value(), pid);

    stackRegion.snapshot();
    auto stackSnap = stackRegion.getLastSnapshot();
    auto stackptr =
        stackSnap->findPointers(loc.back().getActualRegionStart());

    size_t total = 0;
    Log(Message, "Stackptr size: " << stackptr.size());
    total += stackptr.size();
    stackptr =
        stackSnap->findPointers(ptrs.back().getActualRegionStart());
    Log(Message, "Stackptr size: " << stackptr.size());
    total += stackptr.size();
    assert(total > 0,
           "There should be a pointer pointing to the string");
}
