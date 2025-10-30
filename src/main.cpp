#include "snapshots.hpp"
#include "maps.hpp"
#include "core_wrappers.hpp"
#include "core.hpp"
#include "tests.hpp"
#include "logs.hpp"

#include <chrono>
#include <iostream>
#include <queue>
#include <sched.h>
#include <vector>
#include <thread>
#include <unistd.h>
#include <fcntl.h>
using namespace std;
void findPlayerPosition(const size_t &maxSnapshotsPerIteration, RegionPropertiesList &toBeProcessed,
                        QueuedThreadPool &tp
                        ) {
    this_thread::sleep_for(3s);
    do
    {
        size_t numIters =
            toBeProcessed.size() / maxSnapshotsPerIteration;
        numIters = (numIters) ? numIters : 1;
        RegionPropertiesList processing;
        size_t numProcessed = 0;
        for (size_t i = 0; i < numIters; i++)
        {
            size_t               numSnapshots = (i == numIters - 1) ?
                              toBeProcessed.size() - i * maxSnapshotsPerIteration :
                              maxSnapshotsPerIteration;
            RegionPropertiesList currentlyProcessing;

            for (size_t j = 0; j < numSnapshots; j++)
            {
                currentlyProcessing.push_back(toBeProcessed[j]);
            }
            // TODO: Add the ability to create tasks such that
            // each task is roughly similar in size.
            // And then consolidate them later.
            auto inputs = consolidateIntoCoreInput(
                {.mrpVec = currentlyProcessing});
            auto tasks1 =
                createMultipleTasks(makeSnapshotCore, inputs);
            auto tasks2 =
                createMultipleTasks(makeSnapshotCore, inputs);
            tp.submitMultipleTasks(tasks1);
            tp.awaitAllTasks();
            this_thread::sleep_for(500ms);
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

            for (size_t j = 0; j < numSnapshots; j++)
            {
                snapshotsList1.push_back(MemorySnapshot(
                    tasks1[j].result.get(), currentlyProcessing[j],
                    chrono::steady_clock::now().time_since_epoch()));
                spansList1.push_back(
                    snapshotsList1.back().asSnapshotSpan());
                snapshotsList2.push_back(MemorySnapshot(
                    tasks2[j].result.get(), currentlyProcessing[j],
                    chrono::steady_clock::now().time_since_epoch()));
                spansList2.push_back(
                    snapshotsList2.back().asSnapshotSpan());
            }
            // Convert into spans.
            inputs     = consolidateIntoCoreInput({
                    .mrpVec   = currentlyProcessing,
                    .snap1Vec = spansList1,
                    .snap2Vec = spansList2,
            });
            auto tasks = createMultipleTasks(findChangedRegionsCore,
                                             inputs, 8);
            tp.submitMultipleTasks(tasks);
            tp.awaitAllTasks();
            auto changedRegions = consolidateNestedTaskResults(tasks);
            for (auto& region : changedRegions)
            {
                processing.push_back(std::move(region));
            }
            numProcessed += numSnapshots;
            cout << "Processed: " << numProcessed << 
             " out of " << toBeProcessed.size() << "\n";
        }
        std::swap(toBeProcessed, processing);
        processing.clear();
    } while (toBeProcessed.size() > 0);
}

void findPlayerName(const size_t &maxSnapshotsPerIteration, RegionPropertiesList &toBeProcessed,
                        QueuedThreadPool &tp
                        ) {
    size_t numIters =
        toBeProcessed.size() / maxSnapshotsPerIteration;
    numIters = (numIters) ? numIters : 1;
    RegionPropertiesList processing;
    size_t numProcessed = 0;
    for (size_t i = 0; i < numIters; i++)
    {
        size_t               numSnapshots = (i == numIters - 1) ?
                          toBeProcessed.size() - i * maxSnapshotsPerIteration :
                          maxSnapshotsPerIteration;
        RegionPropertiesList currentlyProcessing;

        for (size_t j = 0; j < numSnapshots; j++)
        {
            currentlyProcessing.push_back(toBeProcessed[j]);
        }
        // TODO: Add the ability to create tasks such that
        // each task is roughly similar in size.
        // And then consolidate them later.
        auto inputs = consolidateIntoCoreInput(
            {.mrpVec = currentlyProcessing});
        auto tasks1 =
            createMultipleTasks(makeSnapshotCore, inputs);
        tp.submitMultipleTasks(tasks1);
        tp.awaitAllTasks();

        // Process tasks into snapshots
        vector<MemorySnapshot>     snapshotsList1;
        vector<MemorySnapshotSpan> spansList1;
        snapshotsList1.reserve(tasks1.size());
        spansList1.reserve(tasks1.size());

        for (size_t j = 0; j < numSnapshots; j++)
        {
            snapshotsList1.push_back(MemorySnapshot(
                tasks1[j].result.get(), currentlyProcessing[j],
                chrono::steady_clock::now().time_since_epoch()));
            spansList1.push_back(
                snapshotsList1.back().asSnapshotSpan());
        }
        // Convert into spans.
        inputs     = consolidateIntoCoreInput({
                .mrpVec   = currentlyProcessing,
                .snap1Vec = spansList1,
        });
        auto tasks = createMultipleTasks(findStringCore,
                                         inputs, "Snipingcamper2017");
        tp.submitMultipleTasks(tasks);
        tp.awaitAllTasks();
        auto changedRegions = consolidateNestedTaskResults(tasks);
        for (auto& region : changedRegions)
        {
            processing.push_back(std::move(region));
        }
        numProcessed += numSnapshots;
        cout << "Processed: " << numProcessed << 
         " out of " << toBeProcessed.size() << "\n";
    }
    for (size_t i = 0; i < processing.size(); i++ )
    {
    cout << "Sample: " << processing[i] << endl;
    auto snap = makeSnapshotCore({.mrp = processing[i]});
    for (const char& c: snap) {
        cout << c;
    }
    cout << endl;
    }
    cout << "Number of occurrences: " << processing.size() << endl;
}

using namespace std;
int main(int argc, char* argv[])
{
    Logger::currentLevel = Warning;
    std::string                  bruh;
    if (argc < 2)
    {
        cout << "Not enough arguments! The pid must be an argument"
             << endl;
        return 1;
    }
    pid_t pid = atoi(argv[1]);
    Log(Message, "PID Received: " << pid);
    RegionPropertiesList toBeProcessed;
    {
        auto fullmap  = readMapsFromPid(pid);
        toBeProcessed = breakIntoRegionChunks(
            fullmap
                // .filterRegionsByNotPerms("s")
                // .filterRegionsByPerms("rwp")
                // .filterRegionsByMinSize(0x100000)
                ,
                0
        );
    }

    const size_t   initialNumberOfRegions   = toBeProcessed.size();
    const size_t   numThreads               = 11;
    const size_t   maxSnapshotsPerIteration = 0x30000;

    QueuedThreadPool tp(numThreads);
    vector<RegionPropertiesList> results;


    cout <<
        "Initial number of regions: " << initialNumberOfRegions << endl;
    cout << "Sample: \n" << toBeProcessed.at(0) << endl;
    cout << "Waiting for input: " << endl;
    cin >> bruh;
    // In batches of snapshots, will check each for changes.
    // findPlayerName(maxSnapshotsPerIteration, toBeProcessed, tp);
    findPlayerPosition(maxSnapshotsPerIteration, toBeProcessed, tp);
}
