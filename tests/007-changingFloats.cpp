
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
    auto maps = readMapsFromPid(pid).filterRegionsByPerms("rwp");
    assert(maps.size() > 0, "There should be regions inside the map");
    QueuedThreadPool tp(5);
    {
        auto inputs = consolidateIntoCoreInput({.mrpVec = maps});
        auto tasks1 = createMultipleTasks(makeSnapshotCore, inputs);
        auto tasks2 = createMultipleTasks(makeSnapshotCore, inputs);
        tp.submitMultipleTasks(tasks1);
        tp.awaitAllTasks();
        this_thread::sleep_for(10ms);
        tp.submitMultipleTasks(tasks2);
        tp.awaitAllTasks();

        // Process tasks into snapshots
        vector<MemorySnapshot>     snapshotsList1;
        vector<MemorySnapshot>     snapshotsList2;
        vector<MemorySnapshotSpan> spansList1;
        vector<MemorySnapshotSpan> spansList2;
        snapshotsList1.reserve(tasks1.size());
        snapshotsList2.reserve(tasks2.size());
        spansList1.reserve(tasks1.size());
        spansList2.reserve(tasks2.size());

        for (size_t j = 0; j < tasks1.size(); j++)
        {
            snapshotsList1.push_back(MemorySnapshot(
                tasks1[j].result.get(), maps[j],
                chrono::steady_clock::now().time_since_epoch()));
            spansList1.push_back(
                snapshotsList1.back().asSnapshotSpan());
            snapshotsList2.push_back(MemorySnapshot(
                tasks2[j].result.get(), maps[j],
                chrono::steady_clock::now().time_since_epoch()));
            spansList2.push_back(
                snapshotsList2.back().asSnapshotSpan());
        }
        // Convert into spans.
        inputs = consolidateIntoCoreInput({
            .mrpVec   = maps,
            .snap1Vec = spansList1,
            .snap2Vec = spansList2,
        });
        auto tasks =
            createMultipleTasks(findChangedNumericCore<double>, inputs, 0.5);
        for (auto &task : tasks) {
            task.packagedTask();
        }
        auto changedRegions = consolidateNestedTaskResults(tasks);
        assert(changedRegions.size() > 0,
               "There is a changing double somewhere");
    }
    {
        auto inputs = consolidateIntoCoreInput({.mrpVec = maps});
        auto tasks1 = createMultipleTasks(makeSnapshotCore, inputs);
        auto tasks2 = createMultipleTasks(makeSnapshotCore, inputs);
        tp.submitMultipleTasks(tasks1);
        tp.awaitAllTasks();
        this_thread::sleep_for(10ms);
        tp.submitMultipleTasks(tasks2);
        tp.awaitAllTasks();

        // Process tasks into snapshots
        vector<MemorySnapshot>     snapshotsList1;
        vector<MemorySnapshot>     snapshotsList2;
        vector<MemorySnapshotSpan> spansList1;
        vector<MemorySnapshotSpan> spansList2;
        snapshotsList1.reserve(tasks1.size());
        snapshotsList2.reserve(tasks2.size());
        spansList1.reserve(tasks1.size());
        spansList2.reserve(tasks2.size());

        for (size_t j = 0; j < tasks1.size(); j++)
        {
            snapshotsList1.push_back(MemorySnapshot(
                tasks1[j].result.get(), maps[j],
                chrono::steady_clock::now().time_since_epoch()));
            spansList1.push_back(
                snapshotsList1.back().asSnapshotSpan());
            snapshotsList2.push_back(MemorySnapshot(
                tasks2[j].result.get(), maps[j],
                chrono::steady_clock::now().time_since_epoch()));
            spansList2.push_back(
                snapshotsList2.back().asSnapshotSpan());
        }
        // Convert into spans.
        inputs = consolidateIntoCoreInput({
            .mrpVec   = maps,
            .snap1Vec = spansList1,
            .snap2Vec = spansList2,
        });
        auto tasks =
            createMultipleTasks(findChangedNumericCore<double>, inputs, 0.5);
        tp.submitMultipleTasks(tasks);
        tp.awaitAllTasks();
        auto changedRegions = consolidateNestedTaskResults(tasks);
        assert(changedRegions.size() > 0,
               "There is a changing double somewhere");
    }
    {
        Log(Message, "Testing with broken chunks!");
        auto newMaps =  breakIntoRegionChunks(maps, 0);
        auto inputs = consolidateIntoCoreInput({.mrpVec = newMaps});
        auto tasks1 = createMultipleTasks(makeSnapshotCore, inputs);
        auto tasks2 = createMultipleTasks(makeSnapshotCore, inputs);
        // for (auto &task: tasks1) {
        //     task.packagedTask();
        // }
        tp.submitMultipleTasks(tasks1);
        tp.awaitAllTasks();
        this_thread::sleep_for(10ms);
        // for (auto &task: tasks2) {
        //     task.packagedTask();
        // }
        tp.submitMultipleTasks(tasks2);
        tp.awaitAllTasks();

        // Process tasks into snapshots
        vector<MemorySnapshot>     snapshotsList1;
        vector<MemorySnapshot>     snapshotsList2;
        vector<MemorySnapshotSpan> spansList1;
        vector<MemorySnapshotSpan> spansList2;
        snapshotsList1.reserve(tasks1.size());
        snapshotsList2.reserve(tasks2.size());
        spansList1.reserve(tasks1.size());
        spansList2.reserve(tasks2.size());

        for (size_t j = 0; j < tasks1.size(); j++)
        {
            snapshotsList1.push_back(MemorySnapshot(
                tasks1[j].result.get(), newMaps[j],
                chrono::steady_clock::now().time_since_epoch()));
            spansList1.push_back(
                snapshotsList1.back().asSnapshotSpan());
            snapshotsList2.push_back(MemorySnapshot(
                tasks2[j].result.get(), newMaps[j],
                chrono::steady_clock::now().time_since_epoch()));
            spansList2.push_back(
                snapshotsList2.back().asSnapshotSpan());
        }
        // Convert into spans.
        inputs = consolidateIntoCoreInput({
            .mrpVec   = newMaps,
            .snap1Vec = spansList1,
            .snap2Vec = spansList2,
        });
        auto tasks =
            createMultipleTasks(findChangedNumericCore<double>, inputs, 0.5);
        tp.submitMultipleTasks(tasks);
        tp.awaitAllTasks();
        auto changedRegions = consolidateNestedTaskResults(tasks);
        assert(changedRegions.size() > 0,
               "There is a changing double somewhere");
    }
}
