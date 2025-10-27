#include "core_wrappers.hpp"
#include "logs.hpp"
#include "maps.hpp"
#include "core.hpp"
#include "tests.hpp"
#include "snapshots.hpp"
#include <chrono>

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
                                   .snap1 = snap1.asSpan(),
                                   .snap2 = snap2.asSpan()};
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
        // Double check for sanity.
        MemoryRegionProperties props = map[iter];
        MemorySnapshot snap1(makeSnapshotCore({.mrp = props}), props,
                             steady_clock::now().time_since_epoch());
        this_thread::sleep_for(10ms);
        MemorySnapshot snap2(makeSnapshotCore({.mrp = props}), props,
                             steady_clock::now().time_since_epoch());

        CoreInputs     cInputs = {.mrp   = props,
                                  .snap1 = snap1.asSpan(),
                                  .snap2 = snap2.asSpan()};
        auto changedRegions    = findChangedRegionsCore(cInputs, 8);

        Log(Message,
            "Number of changed regions in region ["
                << props.regionName
                << "] : " << changedRegions.size());

        assert(
            changedRegions.size() > 0,
            "There should be a changing region in the double check!");
    }
    {
        Log(Message, "Testing with pseudo data first");
        // Dividing region with pseudo memory region.
        vector<char> fakeData1 = {
            'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'a', 'b', 'c',
            'd', 'e', 'f', 'g', 'h', 'a', 'b', 'c', 'd', 'e', 'f',
            'g', 'h', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h',
        };
        vector<char> fakeData2 = {
            'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'a', 'b', 'c',
            'd', 'e', 'f', 'g', 'h', 'a', 'b', 'c', 'd', 'e', 'f',
            'g', 'h', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'a',
        };
        MemoryRegionProperties mrp = {
            .perms               = (Perms)11, // Read write private.
            .regionName          = "Test region",
            .parentRegionStart   = 0,
            .parentRegionSize    = 32,
            .relativeRegionStart = 0,
            .relativeRegionSize  = 32,
            .pid                 = -1,
        };
        MemorySnapshot snap1(fakeData1, mrp,
                             steady_clock::now().time_since_epoch());
        this_thread::sleep_for(10ms);
        MemorySnapshot snap2(fakeData2, mrp,
                             steady_clock::now().time_since_epoch());
        size_t         numThreads = 3;
        auto snap1Vec = divideSingleSnapshot(snap1, numThreads);
        auto snap2Vec = divideSingleSnapshot(snap2, numThreads);
        auto mrpVec   = divideSingleRegion(mrp, numThreads);
        auto coreInputsVec = consolidateIntoCoreInput({
            .mrpVec   = mrpVec,
            .snap1Vec = snap1Vec,
            .snap2Vec = snap2Vec,
        });

        auto tasks = createMultipleTasks(findChangedRegionsCore,
                                         coreInputsVec, 8);
        for (auto& task : tasks)
        {
            task.packagedTask();
        }
        auto result = consolidateNestedTaskResults(tasks);
        assert(result.size() > 0,
               "There is definitely a changing region here");
    }
    {
        Log(Message, "Attempting TASK splitting WITHOUT MT!");
        size_t         numThreads = 3;

        MemorySnapshot snap1(makeSnapshotCore({.mrp = map[iter]}),
                             map[iter],
                             steady_clock::now().time_since_epoch());
        this_thread::sleep_for(10ms);
        MemorySnapshot snap2(makeSnapshotCore({.mrp = map[iter]}),
                             map[iter],
                             steady_clock::now().time_since_epoch());

        auto           coreInputsVec = consolidateIntoCoreInput(
            {.mrpVec = divideSingleRegion(map[iter], numThreads),
                       .snap1Vec = divideSingleSnapshot(snap1, numThreads),
                       .snap2Vec = divideSingleSnapshot(snap2, numThreads)});

        auto tasks = createMultipleTasks(findChangedRegionsCore,
                                         coreInputsVec, 8);
        for (auto& task : tasks)
        {
            task.packagedTask();
        }
        auto result = consolidateNestedTaskResults(tasks);
        assert(result.size() > 0,
               "There is definitely a changing region here");
    }
    {
        Log(Message, "Attempting TASK splitting WITH MT!");
        size_t         numThreads = 3;
        MemorySnapshot snap1(makeSnapshotCore({.mrp = map[iter]}),
                             map[iter],
                             steady_clock::now().time_since_epoch());
        this_thread::sleep_for(10ms);
        MemorySnapshot snap2(makeSnapshotCore({.mrp = map[iter]}),
                             map[iter],
                             steady_clock::now().time_since_epoch());
        auto           coreInputsVec = consolidateIntoCoreInput(
            {.mrpVec = divideSingleRegion(map[iter], numThreads),
                       .snap1Vec = divideSingleSnapshot(snap1, numThreads),
                       .snap2Vec = divideSingleSnapshot(snap2, numThreads)});

        auto tasks = createMultipleTasks(findChangedRegionsCore,
                                         coreInputsVec, 8);
        TaskThreadPool tp(numThreads);
        tp.submitMultipleTasks(tasks);
        tp.joinAllThreads();
        auto result = consolidateNestedTaskResults(tasks);
        assert(result.size() > 0,
               "There is definitely a changing region here");
    }
    return 0;
}
