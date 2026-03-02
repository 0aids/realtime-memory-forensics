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
    rmf_Log(rmf_Info, "Testing range numeric search.");
    for (auto &map: maps) {
        auto snap1 = MemorySnapshot::Make(map);
        auto results = findNumeralWithinRange<double>(snap1, 149.9, 150.1);
        if (results.size() > 0) {
            rmf_Log(rmf_Info, "Found double!");
            diffCount += results.size();
        }
    }
    rmf_Log(rmf_Info, "Number of numerics within: " << diffCount);
    rmf_Assert(diffCount > 0, "There is defnitel a double =150");

    diffCount = 0;
    rmf_Log(rmf_Info, "Testing range numeric search (intentional miss).");
    for (auto &map: maps) {
        auto snap1 = MemorySnapshot::Make(map);
        auto results = findNumeralWithinRange<double>(snap1, 150.01, 150.1);
        if (results.size() > 0) {
            rmf_Log(rmf_Warning, "Found double when not supposed to!");
            diffCount += results.size();
        }
    }
    rmf_Log(rmf_Info, "Number of numerics within: " << diffCount);
    rmf_Assert(diffCount == 0, "There are no values between 150.01 and 150.1!");
}
