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
    auto maps = FilterPerms(rmf::utils::ParseMaps(PidToMapsString(samplePid), samplePid), "rwp");
    rmf_Log(rmf_Info, "Number of rwp regions: " << maps.size());
    rmf_Assert(maps.size() > 1, "There should be more than 1 rwp regions...");

    size_t diffCount = 0;
    rmf_Log(rmf_Info, "Testing bitwise Changes!!!");
    for (auto &map: maps) {
        auto snap1 = MemorySnapshot::Make(map);
        this_thread::sleep_for(10ms);
        auto snap2 = MemorySnapshot::Make(map);
        auto results = findChangedRegions(snap1, snap2, 32);
        if (results.size() > 0) {
            rmf_Log(rmf_Info, "Found a difference! Number: " << results.size());
            diffCount += results.size();
        }
    }
    rmf_Log(rmf_Info, "Number of changing regions: " << diffCount);
    rmf_Assert(diffCount > 0, "There are definitely changing regions of memory...");

    // Test changing numerics
    rmf_Log(rmf_Info, "Testing Numerical Changes!!!");
    diffCount = 0;
    for (auto &map: maps) {
        auto snap1 = MemorySnapshot::Make(map);
        this_thread::sleep_for(10ms);
        auto snap2 = MemorySnapshot::Make(map);
        auto results = findNumericChanged<double>(snap1, snap2, 0.5);
        if (results.size() > 0) {
            rmf_Log(rmf_Info, "Found a difference! Number: " << results.size());
            diffCount += results.size();
        }
    }
    rmf_Assert(diffCount > 0, "There should be a changing double...");
    rmf_Log(rmf_Info, "Number of changing regions: " << diffCount);

    rmf_Log(rmf_Info, "Testing Numerical2 Changes!!!");
    diffCount = 0;
    for (auto &map: maps) {
        auto snap1 = MemorySnapshot::Make(map);
        this_thread::sleep_for(10ms);
        auto snap2 = MemorySnapshot::Make(map);
        auto results = findNumericChanged<double>(snap1, snap2, 10);
        if (results.size() > 0) {
            rmf_Log(rmf_Info, "Found a difference! Number: " << results.size());
            diffCount += results.size();
        }
    }
    rmf_Assert(diffCount == 0, "There should be no changing doubles.");
    rmf_Log(rmf_Info, "Number of changes: " << diffCount);

    rmf_Log(rmf_Info, "Testing bitwise UNChanges!!!");

    samplePid = forkTestFunction(unchangingProcess1);
    maps = FilterPerms(rmf::utils::ParseMaps(PidToMapsString(samplePid), samplePid), "rwp");
    rmf_Log(rmf_Info, "Number of rwp regions: " << maps.size());
    rmf_Assert(maps.size() > 1, "There should be more than 1 rwp regions...");

    diffCount = 0;
    for (auto &map: maps) {
        auto snap1 = MemorySnapshot::Make(map);
        this_thread::sleep_for(10ms);
        auto snap2 = MemorySnapshot::Make(map);
        auto results = findChangedRegions(snap1, snap2, 32);
        if (results.size() > 0) {
            rmf_Log(rmf_Warning, "Found a difference when not supposed to!! Number: " << results.size());
            diffCount += results.size();
        }
    }
    rmf_Log(rmf_Info, "Number of changing regions: " << diffCount);
    rmf_Assert(diffCount == 0, "There shouldnt be any differences...");
}
