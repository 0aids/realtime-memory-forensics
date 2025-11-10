#include "backends/mt_backend.hpp"
#include "utils/logs.hpp"
#include "data/maps.hpp"
#include "backends/core.hpp"
#include "tests.hpp"
#include "data/snapshots.hpp"
#include <chrono>

int main()
{
    using namespace std;
    using namespace std::chrono;
    using namespace rmf::data;
    using namespace rmf::backends::core;
    using namespace rmf::backends::mt;
    using namespace rmf::tests;


    auto pid = runSampleProcess();
    this_thread::sleep_for(500ms);
    auto map = readMapsFromPid(pid);
    rmf_assert(map.size() > 0, "There should be regions inside the map");

    rmf_Log(rmf_Message, "Sample of map[0]: \n " << map.front());
    size_t iter = 0;

    // Quickly perform the raw check for the changing region
    {
        rmf_Log(rmf_Message,
            "Checking if there is a changing region of memory!");
        bool   foundChanging = false;
        size_t maxIter       = map.size();

        for (const auto& props : map)
        {
            // if (props.regionName != "UnnamedRegion-3") continue;
            MemorySnapshot snap1(
                makeSnapshotCore({.mrp = props}));
            this_thread::sleep_for(10ms);
            MemorySnapshot snap2(
                makeSnapshotCore({.mrp = props}));

            CoreInputs cInputs  = {.mrp   = props,
                                   .snap1 = snap1.asSnapshotSpan(),
                                   .snap2 = snap2.asSnapshotSpan()};
            auto changedRegions = findChangedRegionsCore(cInputs, 8);

            rmf_Log(rmf_Message,
                "Number of changed regions in region ["
                    << props.regionName
                    << "] : " << changedRegions.size());

            if (changedRegions.size() > 0)
            {
                foundChanging = true;
                rmf_Log(rmf_Message,
                    "Changed region: " << changedRegions.front());
                break;
            }
            if (iter >= maxIter)
            {
                break;
            }
            iter++;
        }
        rmf_assert(
            foundChanging,
            "There should be a changing region somewhere there...");
    }
    {
        // Double check for sanity.
        MemoryRegionProperties props = map[iter];
        MemorySnapshot snap1(makeSnapshotCore({.mrp = props}));
        this_thread::sleep_for(10ms);
        MemorySnapshot snap2(makeSnapshotCore({.mrp = props}));

        CoreInputs     cInputs = {.mrp   = props,
                                  .snap1 = snap1.asSnapshotSpan(),
                                  .snap2 = snap2.asSnapshotSpan()};
        auto changedRegions    = findChangedRegionsCore(cInputs, 8);

        rmf_Log(rmf_Message,
            "Number of changed regions in region ["
                << props.regionName
                << "] : " << changedRegions.size());

        rmf_assert(
            changedRegions.size() > 0,
            "There should be a changing region in the double check!");
    }
    {
        rmf_Log(rmf_Message, "Testing with pseudo data first");
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
        rmf_assert(result.size() > 0,
               "There is definitely a changing region here");
    }
    {
        rmf_Log(rmf_Message, "Attempting TASK splitting WITHOUT MT!");
        size_t         numThreads = 3;

        MemorySnapshot snap1(makeSnapshotCore({.mrp = map[iter]}));
        this_thread::sleep_for(10ms);
        MemorySnapshot snap2(makeSnapshotCore({.mrp = map[iter]}));

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
        rmf_assert(result.size() > 0,
               "There is definitely a changing region here");
    }
    {
        rmf_Log(rmf_Message, "Attempting TASK splitting WITH MT!");
        size_t         numThreads = 3;
        MemorySnapshot snap1(makeSnapshotCore({.mrp = map[iter]}));
        this_thread::sleep_for(10ms);
        MemorySnapshot snap2(makeSnapshotCore({.mrp = map[iter]}));
        auto           coreInputsVec = consolidateIntoCoreInput(
            {.mrpVec = divideSingleRegion(map[iter], numThreads),
                       .snap1Vec = divideSingleSnapshot(snap1, numThreads),
                       .snap2Vec = divideSingleSnapshot(snap2, numThreads)});

        auto tasks = createMultipleTasks(findChangedRegionsCore,
                                         coreInputsVec, 8);
        QueuedThreadPool tp(numThreads);
        tp.submitMultipleTasks(tasks);
        tp.awaitAllTasks();
        auto result = consolidateNestedTaskResults(tasks);
        rmf_assert(result.size() > 0,
               "There is definitely a changing region here");
    }
    return 0;
}
