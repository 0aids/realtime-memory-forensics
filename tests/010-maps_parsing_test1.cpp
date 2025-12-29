#include "types.hpp"
#include "utils.hpp"
#include "logger.hpp"
#include "test_helpers.hpp"

using namespace std;
using namespace rmf::utils;
using namespace rmf::types;

int main() {
    // Test readmaps
    pid_t samplePid = rmf::test::forkTestFunction(rmf::test::changingProcess1);
    auto maps = rmf::utils::ParseMaps(PidToMapsString(samplePid), samplePid);
    rmf_Log(rmf_Info, "Number of regions: " << maps.size());
    rmf_Assert(maps.size() > 1, "There should be more than 1 regions...");

    // Try to take a snapshot of the first region.
    auto map1 = maps[0];
    auto snap = MemorySnapshot::Make(map1);
    rmf_Assert(snap.isValid(), "The snapshot should be valid...");
    snap.printHex(32, 0);
}
