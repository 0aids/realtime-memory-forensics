
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
    rmf_Log(rmf_Info, "Testing string search");
    for (auto &map: maps) {
        auto snap1 = MemorySnapshot::Make(map);
        auto results = findString(snap1, "Lorem ipsum");
        if (results.size() > 0) {
            rmf_Log(rmf_Info, "Found Lorem ipsum!");
            diffCount += results.size();
        }
    }
    rmf_Log(rmf_Info, "Number of lorem ipsums: " << diffCount);
    rmf_Assert(diffCount > 0, "There are definitely lorem ipsums.");

    diffCount = 0;
    rmf_Log(rmf_Info, "Testing intentional non-existance string.");
    for (auto &map: maps) {
        auto snap1 = MemorySnapshot::Make(map);
        auto results = findString(snap1, "peepeepoopoo heeheehahah");
        if (results.size() > 0) {
            rmf_Log(rmf_Warning, "Found non-existant string???");
            diffCount += results.size();
        }
    }
    rmf_Log(rmf_Info, "Number of non-existant strings: " << diffCount);
    rmf_Assert(diffCount == 0, "There shouldn't be any peepeepoopoo hehahes");
}
