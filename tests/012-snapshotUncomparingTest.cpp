#include "types.hpp"
#include "utils.hpp"
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
    auto maps = FilterHasPerms(FilterNotPerms(rmf::utils::ParseMaps(PidToMapsString(samplePid), samplePid), "w"), "r");
    rmf_Log(rmf_Info, "Number of not-writable regions: " << maps.size());
    rmf_Assert(maps.size() > 1, "There should be more than 1 not writeable regions regions...");

    size_t diffCount = 0;
    for (auto &map: maps) {
        auto snap1 = MemorySnapshot::Make(map);
        this_thread::sleep_for(10ms);
        auto snap2 = MemorySnapshot::Make(map);
        auto results = findUnchangedRegions(snap1, snap2, 512);
        if (results.size() > 0) {
            rmf_Log(rmf_Info, "Found a undifference! Number: " << results.size());
            diffCount += results.size();
        }
    }
    rmf_Log(rmf_Info, "Number of changing regions: " << diffCount << " Number of maps: " << maps.size());
    // There are about 6 regions which cannot be read for some reason.
    rmf_Assert(diffCount >= maps.size() - 6, "There is definitely maps.size - ~6 amount of unchanged regions.");
}
