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
    size_t numThreads = thread::hardware_concurrency();
    // Test readmaps
    pid_t samplePid = rmf::test::forkTestFunction(rmf::test::changingProcess1);
    auto maps = FilterHasPerms(rmf::utils::ParseMaps(PidToMapsString(samplePid), samplePid), "r");
    rmf_Log(rmf_Info, "Number of rwp regions: " << maps.size());
    rmf_Assert(maps.size() > 1, "There should be more than 1 rwp regions...");


    // Make all snapshots.
    vector<rmf::Task_t<MemorySnapshot>> snapTasks1;
    vector<rmf::Task_t<MemorySnapshot>> snapTasks2;
    rmf::TaskThreadPool_t tp(numThreads / 2);

    for (size_t i = 0; i < maps.size(); i++) 
    {
        snapTasks1.emplace_back(
            MemorySnapshot::Make, maps[i]
        );
        snapTasks2.emplace_back(
            MemorySnapshot::Make, maps[i]
        );
    }
    tp.SubmitMultipleTasks(snapTasks1);
    tp.AwaitTasks();
    this_thread::sleep_for(500ms);
    tp.SubmitMultipleTasks(snapTasks2);
    tp.AwaitTasks();

    vector<rmf::Task_t<MemoryRegionPropertiesVec>> compareTasks;
    // rmf_Log(rmf_Info, "Sample hex");
    // snapTasks1[0].getFuture().get().printHex(32, 10);

    for (size_t i = 0; i < snapTasks1.size(); i++) {
        compareTasks.emplace_back(findChangedRegions, snapTasks1[i].getFuture().get(), snapTasks2[i].getFuture().get(), 32);
    }

    tp.SubmitMultipleTasks(compareTasks);
    tp.AwaitTasks();
    MemoryRegionPropertiesVec results;
    // Consolidate
    for (auto &task : compareTasks) {
        auto tempRes = task.getFuture().get();
        std::copy(tempRes.begin(), tempRes.end(), back_inserter(results));
    }

    rmf_Log(rmf_Info, "Number of changes: " << results.size());
    rmf_Assert(results.size() > 0, "There are changing regions in there...");

    for (size_t i = 0; i < maps.size(); i++) {
    }
}
