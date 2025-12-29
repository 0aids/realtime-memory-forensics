#include "types.hpp"
#include "utils.hpp"
#include "logger.hpp"
#include "test_helpers.hpp"

using namespace std;
using namespace rmf::utils;
using namespace rmf::types;

int main() {
    // Test readmaps
    pid_t samplePid = rmf::test::startTestFunction();
    auto maps = rmf::utils::ParseMaps(PidToMapsString(samplePid));
    rmf_Log(rmf_Info, "Number of regions: " << maps.size());
    rmf_Assert(maps.size() > 1, "There should be more than 1 regions...");

    // Find a string
    for (auto &map: maps) {
    }
}
