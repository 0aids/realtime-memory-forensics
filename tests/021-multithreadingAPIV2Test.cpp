#include "rmf.hpp"
#include "types.hpp"
#include "utils.hpp"
#include <iterator>
#include <vector>
#include "multi_threading.hpp"
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
    size_t numThreads = thread::hardware_concurrency() / 2;
    pid_t samplePid = rmf::test::forkTestFunction(rmf::test::changingProcess1);
    auto maps = FilterHasPerms(rmf::utils::ParseMaps(PidToMapsString(samplePid), samplePid), "r");
    rmf_Log(rmf_Info, "Number of regions: " << maps.size());
    rmf_Assert(maps.size() > 1, "There should be more than 1 memory regions...");
    auto analyzer = rmf::Analyzer(numThreads);

    auto snapshots = analyzer.Execute(MemorySnapshot::Make, maps);
    rmf_Log(rmf_Info, "Number of snapshots: " << snapshots.size());
    rmf_Assert(snapshots.size() > 1, "There should be more than 1 snapshotted region...");
}
