#include "core_wrappers.hpp"
#include "logs.hpp"
#include "maps.hpp"
#include "core.hpp"
#include "tests.hpp"
#include "snapshots.hpp"
#include <chrono>
#include <thread>

int main()
{
    using namespace std;
    using namespace std::chrono;

    auto pid = runSampleProcess();
    this_thread::sleep_for(500ms);
    auto map = readMapsFromPid(pid);
    assert(map.size() > 0, "There should be regions inside the map");

    Log(Message, "Sample of map[0]: \n " << map.front());
    size_t iter = 0;

    // Quickly perform the raw check for the changing region
    {
        Log(Message,
            "Checking if there is a changing region of memory!");
        bool   foundChanging = false;
        size_t maxIter       = map.size();

        for (const auto& props : map)
        {
            // if (props.regionName != "UnnamedRegion-3") continue;
            MemorySnapshot snap1(
                makeSnapshotCore({.mrp = props}), props,
                steady_clock::now().time_since_epoch());
            this_thread::sleep_for(10ms);
            MemorySnapshot snap2(
                makeSnapshotCore({.mrp = props}), props,
                steady_clock::now().time_since_epoch());

            CoreInputs cInputs  = {.mrp   = props,
                                   .snap1 = snap1.asSnapshotSpan(),
                                   .snap2 = snap2.asSnapshotSpan()};
            auto changedRegions = findChangedRegionsCore(cInputs, 8);

            Log(Message,
                "Number of changed regions in region ["
                    << props.regionName
                    << "] : " << changedRegions.size());

            if (changedRegions.size() > 0)
            {
                foundChanging = true;
                Log(Message,
                    "Changed region: " << changedRegions.front());
                break;
            }
            if (iter >= maxIter)
            {
                break;
            }
            iter++;
        }
        assert(
            foundChanging,
            "There should be a changing region somewhere there...");
    }
    {
        const size_t numThreads = 5;
        bool foundChanging = false;
        const size_t numIters = 10;
        const size_t regionsPerIter = map.size() / numIters;
        QueuedThreadPool tp(numThreads);

        for (size_t i = 0; i < map.size(); i+=regionsPerIter) {
            RegionPropertiesList mrpVec;

            for (size_t j = i; j < i + regionsPerIter && j < map.size(); j++ ){
                mrpVec.push_back(map[j]);
            }

            auto coreInputsVec = consolidateIntoCoreInput({.mrpVec=mrpVec});

            auto tasks1 = createMultipleTasks(makeSnapshotCore, coreInputsVec);
            auto tasks2 = createMultipleTasks(makeSnapshotCore, coreInputsVec);

            // for (auto &task: tasks1) {
            //     task.packagedTask();
            // }

            // for (auto &task: tasks2) {
            //     task.packagedTask();
            // }
            tp.submitMultipleTasks(tasks1);
            tp.awaitAllTasks();
            this_thread::sleep_for(10ms);
            tp.submitMultipleTasks(tasks2);
            tp.awaitAllTasks();

            std::vector<MemorySnapshot> snapshotsVec1;
            snapshotsVec1.reserve(regionsPerIter);

            std::vector<MemorySnapshot> snapshotsVec2;
            snapshotsVec2.reserve(regionsPerIter);

            for (size_t it = 0; it < tasks1.size(); it++) {
                Log(Debug, "Mrp: " << coreInputsVec[it].mrp.value());
                snapshotsVec1.push_back({tasks1[it].result.get(), coreInputsVec[it].mrp.value(), 100ms});
                snapshotsVec2.push_back({tasks2[it].result.get(), coreInputsVec[it].mrp.value(), 100ms});
            }

            auto comparisonVec = consolidateIntoCoreInput(
                {
                    .mrpVec = mrpVec,
                    .snap1Vec = divideMultipleSnapshots(snapshotsVec1),
                    .snap2Vec = divideMultipleSnapshots(snapshotsVec2)
                }
            );

            auto tasks = createMultipleTasks(findChangedRegionsCore, comparisonVec, 8); 
            // for (auto &task: tasks) {
            //     task.packagedTask();
            // }
            tp.submitMultipleTasks(tasks);
            tp.awaitAllTasks();
            auto result = consolidateNestedTaskResults(tasks);
            if (result.size() > 0) {
                Log(Message, "Found changing region!");
                foundChanging = true;
                break;
            }
        }
        assert(foundChanging, "There should be a changing region in here");
    }
    return 0;
}
