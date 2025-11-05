#include "snapshots.hpp"
#include "maps.hpp"
#include "core_wrappers.hpp"
#include "core.hpp"
#include "tests.hpp"
#include "logs.hpp"
#include "imgui.h"
#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_sdl3.h"
#include "gui.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>

#include <chrono>
#include <iomanip>
#include <set>
#include <cstdint>
#include <iostream>
#include <queue>
#include <sched.h>
#include <vector>
#include <thread>
#include <unistd.h>
#include <fcntl.h>
using namespace std::chrono;
using namespace std;

template <Numeric NumType>
NumType bitShift(NumType val, ssize_t bs)
{
    long* l = (long*)&val;
    *l <<= bs;
    return *(NumType*)l;
}

void findPlayerPosition(const size_t& maxSnapshotsPerIteration,
                        RegionPropertiesList& toBeProcessed,
                        QueuedThreadPool&     tp)
{
    string bruh;
    double minDiff = 3;
    double maxDiff = 0.1;
    cout << std::dec;
    do
    {
        cout << "Changing move test... enter somthing to cont (q to "
                "quit): ";
        cin >> bruh;
        if (bruh == "q")
            break;
        this_thread::sleep_for(3s);
        cout << "NOW!" << endl;
        size_t consecNoChange = 0;
        size_t last           = toBeProcessed.size();
        // while (consecNoChange < 10)
        {
            size_t numIters =
                toBeProcessed.size() / maxSnapshotsPerIteration;
            numIters = (numIters) ? numIters : 1;
            RegionPropertiesList processing;
            size_t               numProcessed = 0;
            for (size_t i = 0; i < numIters; i++)
            {
                size_t numSnapshots = (i == numIters - 1) ?
                    toBeProcessed.size() -
                        i * maxSnapshotsPerIteration :
                    maxSnapshotsPerIteration;
                RegionPropertiesList currentlyProcessing;

                for (size_t j = 0; j < numSnapshots; j++)
                {
                    currentlyProcessing.push_back(toBeProcessed[j]);
                }
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
                    snapshotsList1.push_back(
                        MemorySnapshot(tasks1[j].result.get()));
                    spansList1.push_back(
                        snapshotsList1.back().asSnapshotSpan());
                    snapshotsList2.push_back(
                        MemorySnapshot(tasks2[j].result.get()));
                    spansList2.push_back(
                        snapshotsList2.back().asSnapshotSpan());
                }
                // Convert into spans.
                inputs = consolidateIntoCoreInput({
                    .mrpVec   = currentlyProcessing,
                    .snap1Vec = spansList1,
                    .snap2Vec = spansList2,
                });
                // auto tasks = createMultipleTasks(
                //     findChangedNumericCore<double>, inputs, minDiff);
                auto tasks = createMultipleTasks(
                    findChangedRegionsCore, inputs, 4);
                tp.submitMultipleTasks(tasks);
                tp.awaitAllTasks();
                auto changedRegions =
                    consolidateNestedTaskResults(tasks);
                for (auto& region : changedRegions)
                {
                    processing.push_back(std::move(region));
                }
                numProcessed += numSnapshots;
                cout << "Processed: " << numProcessed << " out of "
                     << toBeProcessed.size() << "\n";
            }
            std::swap(toBeProcessed, processing);
            cout << "Number left: " << toBeProcessed.size() << endl;
            processing.clear();
            if (toBeProcessed.size() != last)
            {
                consecNoChange = 0;
            }
            else
            {
                consecNoChange += 1;
            }
            last = toBeProcessed.size();
        }
        cout << "UNCHanging test... enter somthing to cont (q to "
                "quit): ";
        cin >> bruh;
        if (bruh == "q")
            break;
        this_thread::sleep_for(1s);
        {
            size_t numIters =
                toBeProcessed.size() / maxSnapshotsPerIteration;
            numIters = (numIters) ? numIters : 1;
            RegionPropertiesList processing;
            size_t               numProcessed = 0;
            for (size_t i = 0; i < numIters; i++)
            {
                size_t numSnapshots = (i == numIters - 1) ?
                    toBeProcessed.size() -
                        i * maxSnapshotsPerIteration :
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
                    snapshotsList1.push_back(
                        tasks1[j].result.get()
                    );
                    spansList1.push_back(
                        snapshotsList1.back().asSnapshotSpan());
                    snapshotsList2.push_back(
                        tasks2[j].result.get()
                    );
                    spansList2.push_back(
                        snapshotsList2.back().asSnapshotSpan());
                }
                // Convert into spans.
                inputs = consolidateIntoCoreInput({
                    .mrpVec   = currentlyProcessing,
                    .snap1Vec = spansList1,
                    .snap2Vec = spansList2,
                });
                // auto tasks = createMultipleTasks(
                //     findUnchangedNumericCore<double>, inputs, maxDiff);
                auto tasks = createMultipleTasks(
                    findUnchangedRegionsCore, inputs, 4);
                tp.submitMultipleTasks(tasks);
                tp.awaitAllTasks();
                auto changedRegions =
                    consolidateNestedTaskResults(tasks);
                for (auto& region : changedRegions)
                {
                    processing.push_back(std::move(region));
                }
                numProcessed += numSnapshots;
                cout << "Processed: " << numProcessed << " out of "
                     << toBeProcessed.size() << "\n";
            }
            std::swap(toBeProcessed, processing);
            processing.clear();
            cout << "Number left: " << toBeProcessed.size() << endl;
        }

        cout << "Finding within a good range" << endl;
        cout << "enter to cont, q to quit: ";
        cin >> bruh;
        if (bruh == "q")
            break;
        {
            size_t numIters =
                toBeProcessed.size() / maxSnapshotsPerIteration;
            numIters = (numIters) ? numIters : 1;
            RegionPropertiesList processing;
            size_t               numProcessed = 0;
            for (size_t i = 0; i < numIters; i++)
            {
                size_t numSnapshots = (i == numIters - 1) ?
                    toBeProcessed.size() -
                        i * maxSnapshotsPerIteration :
                    maxSnapshotsPerIteration;
                RegionPropertiesList currentlyProcessing;

                for (size_t j = 0; j < numSnapshots; j++)
                {
                    currentlyProcessing.push_back(toBeProcessed[j]);
                }
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
                    snapshotsList1.push_back(
                        MemorySnapshot(tasks1[j].result.get()));
                    spansList1.push_back(
                        snapshotsList1.back().asSnapshotSpan());
                }
                // Convert into spans.
                inputs = consolidateIntoCoreInput({
                    .mrpVec   = currentlyProcessing,
                    .snap1Vec = spansList1,
                });
                auto tasks =
                    createMultipleTasks(findNumericWithinRange<float>,
                                        inputs, 59.5f, 62.f);
                tp.submitMultipleTasks(tasks);
                tp.awaitAllTasks();
                auto changedRegions =
                    consolidateNestedTaskResults(tasks);
                for (auto& region : changedRegions)
                {
                    processing.push_back(std::move(region));
                }
                numProcessed += numSnapshots;
                cout << "Processed: " << numProcessed << " out of "
                     << toBeProcessed.size() << "\n";
            }
            std::swap(toBeProcessed, processing);
            cout << "Number left: " << toBeProcessed.size() << endl;
            processing.clear();
            if (toBeProcessed.size() != last)
            {
                consecNoChange = 0;
            }
            else
            {
                consecNoChange += 1;
            }
            last = toBeProcessed.size();
        }

    } while (toBeProcessed.size() > 0);
    // Take a samples as float64 and continuously print it out.
    if (toBeProcessed.size() == 0)
        return;
    cout << std::dec;
    // cout << "Printing regions!" << endl;
    // for (auto &a : toBeProcessed) {
    //     cout << a << endl;;
    // }
    cout << "Eliminating duplicate regions!" << endl;
    set<MemoryRegionProperties> setMrp(toBeProcessed.begin(),
                                       toBeProcessed.end());
    RegionPropertiesList        rl;
    for (auto& mrp : setMrp)
    {
        rl.push_back(mrp);
    }
    cout << "Remaining regions: " << rl.size() << endl;
    while (true)
    {
        cout << "Choose your index out of " << rl.size() << " : ";
        size_t index;
        cin >> index;
        cout << "Specified region: \n" << rl[index] << endl;
        cout << std::dec;
        for (size_t i = 0; i < 10; i++)
        {
            CoreInputs input = {.mrp = rl[index]};
            auto       task  = createTask(makeSnapshotCore, input);
            tp.submitTask(task);
            tp.awaitAllTasks();
            auto  data = task.result.get();
            float dvalue;
            int   ival;
            memcpy(&dvalue, data.data(), sizeof(float));
            memcpy(&ival, data.data(), sizeof(int));
            cout << "as float:\t" << setprecision(5) << dvalue;
            cout << "\tas int:\t" << ival << endl;
            // cout << " As bitshifted by " << bs
            //      << ":\t" << bitShift(dvalue, bs) << endl;
            // cout << "as int64:\t" << ivalue << endl;
            // cout << "as uint64t:\t" << uvalue << endl;
            this_thread::sleep_for(500ms);
        }
    }
}

void findPlayerName(const size_t&         maxSnapshotsPerIteration,
                    RegionPropertiesList& toBeProcessed,
                    QueuedThreadPool&     tp)
{
    size_t numIters = toBeProcessed.size() / maxSnapshotsPerIteration;
    numIters        = (numIters) ? numIters : 1;
    RegionPropertiesList processing;
    size_t               numProcessed = 0;
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
        auto inputs =
            consolidateIntoCoreInput({.mrpVec = currentlyProcessing});
        auto tasks1 = createMultipleTasks(makeSnapshotCore, inputs);
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
                tasks1[j].result.get()));
            spansList1.push_back(
                snapshotsList1.back().asSnapshotSpan());
        }
        // Convert into spans.
        inputs     = consolidateIntoCoreInput({
                .mrpVec   = currentlyProcessing,
                .snap1Vec = spansList1,
        });
        auto tasks = createMultipleTasks(findStringCore, inputs,
                                         "Snipingcamper2017");
        tp.submitMultipleTasks(tasks);
        tp.awaitAllTasks();
        auto changedRegions = consolidateNestedTaskResults(tasks);
        for (auto& region : changedRegions)
        {
            processing.push_back(std::move(region));
        }
        numProcessed += numSnapshots;
        cout << "Processed: " << numProcessed << " out of "
             << toBeProcessed.size() << "\n";
    }
    for (size_t i = 0; i < processing.size(); i++)
    {
        cout << "Sample: " << processing[i] << endl;
        auto snap = makeSnapshotCore({.mrp = processing[i]});
        for (const char& c : snap)
        {
            cout << c;
        }
        cout << endl;
    }
    cout << "Number of occurrences: " << processing.size() << endl;
}



// Main code
int main(int, char**)
{
    pid_t                samplePid = runSampleProcess();
    RegionPropertiesList map =
        readMapsFromPid(samplePid).filterRegionsByPerms("rwp");

    RegionPropertiesList result;
    {
        Log(Message, "Attempting TASK splitting WITH MT!");
        size_t numThreads = 10;
        map               = breakIntoRegionChunks(map, 0);
        QueuedThreadPool tp(numThreads);

        auto cinputs = consolidateIntoCoreInput({.mrpVec = map});
        auto snapTasks1 =
            createMultipleTasks(makeSnapshotCore, cinputs);
        auto snapTasks2 =
            createMultipleTasks(makeSnapshotCore, cinputs);

        tp.submitMultipleTasks(snapTasks1);
        tp.awaitAllTasks();
        this_thread::sleep_for(10ms);
        tp.submitMultipleTasks(snapTasks2);
        tp.awaitAllTasks();

        vector<MemorySnapshot>     snapshotsList1;
        vector<MemorySnapshot>     snapshotsList2;
        vector<MemorySnapshotSpan> spansList1;
        vector<MemorySnapshotSpan> spansList2;
        snapshotsList1.reserve(snapTasks1.size());
        snapshotsList2.reserve(snapTasks2.size());
        spansList1.reserve(snapTasks1.size());
        spansList2.reserve(snapTasks2.size());

        for (size_t j = 0; j < snapTasks1.size(); j++)
        {
            snapshotsList1.push_back(MemorySnapshot(
                snapTasks1[j].result.get()));
            spansList1.push_back(
                snapshotsList1.back().asSnapshotSpan());
            snapshotsList2.push_back(MemorySnapshot(
                snapTasks2[j].result.get()));
            spansList2.push_back(
                snapshotsList2.back().asSnapshotSpan());
        }
        cinputs = consolidateIntoCoreInput({.mrpVec   = map,
                                            .snap1Vec = spansList1,
                                            .snap2Vec = spansList2});

        auto tasks =
            createMultipleTasks(findChangedRegionsCore, cinputs, 8);
        tp.submitMultipleTasks(tasks);
        tp.awaitAllTasks();
        result = consolidateNestedTaskResults(tasks);
    }

    RefreshableSnapshot rs( breakIntoRegionChunks(map, 0).front()); 
    RefreshableSnapshot crs(result.front());

    RefreshableSnapshotMenu rsms(rs);

    RefreshableSnapshotMenu crsms(crs);

    // Default construct the gui state.
    GuiState gs{};
    if (!gs.validState) {
        Log(Error, "Failed to initalize the GUI!");
        return 1;
    }

    // Main loop
    bool done = false;
    while (!done)
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        // [If using SDL_MAIN_USE_CALLBACKS: call ImGui_ImplSDL3_ProcessEvent() from your SDL_AppEvent() function]
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            // Must be here to interrupt events?
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT)
                done = true;
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
                event.window.windowID == SDL_GetWindowID(gs.window))
                done = true;
        }

        // [If using SDL_MAIN_USE_CALLBACKS: all code below would likely be your SDL_AppIterate() function]
        if (SDL_GetWindowFlags(gs.window) & SDL_WINDOW_MINIMIZED)
        {
            SDL_Delay(10);
            continue;
        }
        // Imgui shit

        gs.startFrame();

        gs.draw();

        rsms.draw();
        crsms.draw();


        gs.endFrame();
    }

    // Cleanup
    // [If using SDL_MAIN_USE_CALLBACKS: all code below would likely be your SDL_AppQuit() function]
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DestroyContext(gs.glContext);
    SDL_DestroyWindow(gs.window);
    SDL_Quit();

    return 0;
}

// int main(int argc, char* argv[])
// {
//     Logger::currentLevel = Warning;
//     std::string bruh;
//     Log(Warning, "Test warning");
//     Log(Error, "Test error");
//     Log(Message, "Test message");
//     if (argc < 2)
//     {
//         cout << "Not enough arguments! The pid must be an argument"
//              << endl;
//         return 1;
//     }
//     pid_t pid = atoi(argv[1]);
//     Log(Message, "PID Received: " << pid);
//     const size_t     numThreads               = 11;
//     QueuedThreadPool tp(numThreads);
//     RegionPropertiesList toBeProcessed;
//     {
//         auto fullmap = readMapsFromPid(pid);
//         toBeProcessed =
//             getActiveRegionsFromRegionPropertiesList(fullmap
//                                   // .filterRegionsByNotPerms("s")
//                                   .filterRegionsByPerms("rwp")
//                                   // .filterRegionsByMinSize(0x100000)
//                                   );
//     }

//     const size_t     initialNumberOfRegions   = toBeProcessed.size();
//     const size_t     maxSnapshotsPerIteration = 0x30000;

//     vector<RegionPropertiesList> results;

//     cout << "Initial number of regions: " << initialNumberOfRegions
//          << endl;
//     cout << "Sample: \n" << toBeProcessed.back() << endl;
//     cout << "Waiting for input: " << endl;
//     cin >> bruh;
//     // In batches of snapshots, will check each for changes.
//     // findPlayerName(maxSnapshotsPerIteration, toBeProcessed, tp);
//     findPlayerPosition(maxSnapshotsPerIteration, toBeProcessed, tp);
//     // MemoryRegionProperties playerPos = {
//     //     .perms = Perms::READ | Perms::WRITE | Perms::PRIVATE,
//     //     .regionName = "UnnamedRegion-12",
//     //     .parentRegionStart =  0x7fcba8500000,
//     //     .parentRegionSize = 0x40000000,
//     //     .relativeRegionStart = 0x1825a4,
//     //     .relativeRegionSize = 0x4,
//     //     .pid = pid,
//     // };
//     // while (true) {
//     //     CoreInputs input = {.mrp = playerPos};
//     //     auto task =
//     //         createTask(makeSnapshotCore, input);
//     //     tp.submitTask(task);
//     //     tp.awaitAllTasks();
//     //     auto  data = task.result.get();
//     //     float dvalue;
//     //     memcpy(&dvalue, data.data(), sizeof(float));
//     //     cout << "\33[2K\rPosition: " << setprecision(10) << dvalue;
//     //     std::flush(cout);
//     //     this_thread::sleep_for(5ms);
//     // }
// }
