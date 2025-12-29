#include "types.hpp"
#include "utils.hpp"
#include <cstring>
#include <thread>
#include "logger.hpp"
#include "test_helpers.hpp"
#include "operations.hpp"

using namespace std;
using namespace rmf::utils;
using namespace rmf::types;
using namespace rmf::op;
using namespace rmf::test;

int main() {
    // Test readmaps
    pid_t samplePid = rmf::test::forkTestFunction(rmf::test::changingProcess1);
    auto maps = FilterHasPerms(rmf::utils::ParseMaps(PidToMapsString(samplePid), samplePid), "r");
    rmf_Log(rmf_Info, "Number of rwp regions: " << maps.size());
    rmf_Assert(maps.size() > 1, "There should be more than 1 rwp regions...");

    rmf_Log(rmf_Info, "Testing range numeric search.");
    MemoryRegionProperties mrp;
    bool found = false;
    for (auto &map: maps) {
        auto snap1 = MemorySnapshot::Make(map);
        auto results = findNumeralWithinRange<double>(snap1, 149.9, 150.1);
        if (results.size() > 0) {
            // Use this double to figure out what points to it.
            rmf_Log(rmf_Info, "Found double!");
            mrp = results.back();
            found = true;
        }
    }
    rmf_Assert(found, "There should be a double equal to 150...");
    rmf_Log(rmf_Info, "Searching for a pointer that points to that double...");
    // Double check manually that it is 150.
    auto snap = MemorySnapshot::Make(mrp);
    auto span = snap.getData();
    rmf_Log(rmf_Info, "mrp: " << mrp.toString());
    double value;
    memcpy(&value, span.data(), sizeof(double));
    rmf_Log(rmf_Info, "Value: " << value);

    found = false;
    // mrp.relativeRegionAddress -= sizeof(double) * 3;
    // mrp.relativeRegionSize *= 8;
    // rmf_Log(rmf_Info, "Modified mrp: " << mrp.toString());
    for (auto &map: maps) {
        auto snap1 = MemorySnapshot::Make(map);
        auto results = findPointersToRegion(snap1, mrp);
        if (results.size() > 0) {
            // Use this double to figure out what points to it.
            rmf_Log(rmf_Info, "Found pointer!");
            found = true;
            mrp = results.back();
        }
    }
    rmf_Assert(found, "There is definitely a pointer to the 150 double...");
    rmf_Log(rmf_Info, "New mrp: " << mrp.toString());
}
