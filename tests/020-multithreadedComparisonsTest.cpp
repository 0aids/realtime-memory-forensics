#include "types.hpp"
#include "utils.hpp"
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
    vector<rmf::Task_t<std::function<decltype(MemorySnapshot::Make)>>> snapTasks1;
    vector<rmf::Task_t<std::function<decltype(MemorySnapshot::Make)>>> snapTasks2;
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
    tp.SubmitMultipleTasks(snapTasks2);
    tp.AwaitTasks();
}
