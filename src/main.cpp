#include "snapshots.hpp"
#include "maps.hpp"
#include "core_wrappers.hpp"
#include "core.hpp"
#include "tests.hpp"
#include "logs.hpp"
#include "imgui.h"
#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_sdl3.h"
#include <SDL3/SDL.h>

#include <chrono>
#include <set>
#include <cstdint>
#include <iostream>
#include <queue>
#include <sched.h>
#include <vector>
#include <thread>
#include <unistd.h>
#include <fcntl.h>
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
                        MemorySnapshot(tasks1[j].result.get(),
                                       currentlyProcessing[j],
                                       chrono::steady_clock::now()
                                           .time_since_epoch()));
                    spansList1.push_back(
                        snapshotsList1.back().asSnapshotSpan());
                    snapshotsList2.push_back(
                        MemorySnapshot(tasks2[j].result.get(),
                                       currentlyProcessing[j],
                                       chrono::steady_clock::now()
                                           .time_since_epoch()));
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
        // {
        //     cout << "Finding within a good range" << endl;
        //     size_t numIters =
        //         toBeProcessed.size() / maxSnapshotsPerIteration;
        //     numIters = (numIters) ? numIters : 1;
        //     RegionPropertiesList processing;
        //     size_t               numProcessed = 0;
        //     for (size_t i = 0; i < numIters; i++)
        //     {
        //         size_t numSnapshots = (i == numIters - 1) ?
        //             toBeProcessed.size() -
        //                 i * maxSnapshotsPerIteration :
        //             maxSnapshotsPerIteration;
        //         RegionPropertiesList currentlyProcessing;

        //         for (size_t j = 0; j < numSnapshots; j++)
        //         {
        //             currentlyProcessing.push_back(toBeProcessed[j]);
        //         }
        //         auto inputs = consolidateIntoCoreInput(
        //             {.mrpVec = currentlyProcessing});
        //         auto tasks1 =
        //             createMultipleTasks(makeSnapshotCore, inputs);
        //         tp.submitMultipleTasks(tasks1);
        //         tp.awaitAllTasks();

        //         // Process tasks into snapshots
        //         vector<MemorySnapshot>     snapshotsList1;
        //         vector<MemorySnapshotSpan> spansList1;
        //         snapshotsList1.reserve(tasks1.size());
        //         spansList1.reserve(tasks1.size());

        //         for (size_t j = 0; j < numSnapshots; j++)
        //         {
        //             snapshotsList1.push_back(
        //                 MemorySnapshot(tasks1[j].result.get(),
        //                                currentlyProcessing[j],
        //                                chrono::steady_clock::now()
        //                                    .time_since_epoch()));
        //             spansList1.push_back(
        //                 snapshotsList1.back().asSnapshotSpan());
        //         }
        //         // Convert into spans.
        //         inputs     = consolidateIntoCoreInput({
        //                 .mrpVec   = currentlyProcessing,
        //                 .snap1Vec = spansList1,
        //         });
        //         auto tasks = createMultipleTasks(
        //             findNumericWithinRange<double> , inputs, 1e4, -1e4);
        //         tp.submitMultipleTasks(tasks);
        //         tp.awaitAllTasks();
        //         auto changedRegions =
        //             consolidateNestedTaskResults(tasks);
        //         for (auto& region : changedRegions)
        //         {
        //             processing.push_back(std::move(region));
        //         }
        //         numProcessed += numSnapshots;
        //         cout << "Processed: " << numProcessed << " out of "
        //              << toBeProcessed.size() << "\n";
        //     }
        //     std::swap(toBeProcessed, processing);
        //     cout << "Number left: " << toBeProcessed.size() << endl;
        //     processing.clear();
        //     if (toBeProcessed.size() != last) {
        //         consecNoChange = 0;
        //     } else {
        //         consecNoChange += 1;
        //     }
        //     last = toBeProcessed.size();
        // }
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
                        MemorySnapshot(tasks1[j].result.get(),
                                       currentlyProcessing[j],
                                       chrono::steady_clock::now()
                                           .time_since_epoch()));
                    spansList1.push_back(
                        snapshotsList1.back().asSnapshotSpan());
                    snapshotsList2.push_back(
                        MemorySnapshot(tasks2[j].result.get(),
                                       currentlyProcessing[j],
                                       chrono::steady_clock::now()
                                           .time_since_epoch()));
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
    set<MemoryRegionProperties> setMrp(toBeProcessed.begin(), toBeProcessed.end());
    RegionPropertiesList rl;
    for (auto &mrp : setMrp) {
        rl.push_back(mrp);
    }
    cout << "Remaining regions: " << rl.size() << endl;
    while (true)
    {
        cout << "Choose your index out of " << rl.size()
             << " : ";
        size_t index;
        cin >> index;
        cout << "Specified region: \n"
             << rl[index] << endl;
        cout << std::dec;
        for (size_t i = 0; i < 10; i++)
        {
            CoreInputs input = {.mrp = rl[index]};
            auto task =
                createTask(makeSnapshotCore, input);
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

// int main()
// {
//     const char* glsl_version = "#version 300 es";
//     SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
//     SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
//                         SDL_GL_CONTEXT_PROFILE_ES);
//     SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
//     SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

//     // Create window with graphics context
//     SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
//     SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
//     SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
//     float main_scale =
//         SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
//     SDL_WindowFlags window_flags = SDL_WINDOW_OPENGL |
//         SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN |
//         SDL_WINDOW_HIGH_PIXEL_DENSITY;
//     SDL_Window* window = SDL_CreateWindow(
//         "Dear ImGui SDL3+OpenGL3 example", (int)(1280 * main_scale),
//         (int)(800 * main_scale), window_flags);
//     if (window == nullptr)
//     {
//         printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
//         return 1;
//     }
//     SDL_GLContext gl_context = SDL_GL_CreateContext(window);
//     if (gl_context == nullptr)
//     {
//         printf("Error: SDL_GL_CreateContext(): %s\n", SDL_GetError());
//         return 1;
//     }

//     SDL_GL_MakeCurrent(window, gl_context);
//     SDL_GL_SetSwapInterval(1); // Enable vsync
//     SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
//     SDL_ShowWindow(window);
//     // Setup Dear ImGui context
//     IMGUI_CHECKVERSION();
//     ImGui::CreateContext();
//     ImGuiIO& io = ImGui::GetIO(); (void)io;
//     io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
//     io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

//     // Setup Dear ImGui style
//     ImGui::StyleColorsDark();
//     //ImGui::StyleColorsLight();

//     // Setup scaling
//     ImGuiStyle& style = ImGui::GetStyle();
//     style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
//     style.FontScaleDpi = main_scale;        // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this unnecessary. We leave both here for documentation purpose)

//     // Setup Platform/Renderer backends
//     ImGui_ImplSDL3_InitForOpenGL(window, gl_context);
//     ImGui_ImplOpenGL3_Init(glsl_version);

// }

int main(int argc, char* argv[])
{
    Logger::currentLevel = Warning;
    std::string bruh;
    Log(Warning, "Test warning");
    Log(Error, "Test error");
    Log(Message, "Test message");
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
        auto fullmap = readMapsFromPid(pid);
        toBeProcessed =
            getActiveRegionsFromRegionPropertiesList(fullmap
                                  .filterRegionsByNotPerms("s")
                                  // .filterRegionsByPerms("rwp")
                                  // .filterRegionsByMinSize(0x100000)
                                  );
    }

    const size_t     initialNumberOfRegions   = toBeProcessed.size();
    const size_t     numThreads               = 11;
    const size_t     maxSnapshotsPerIteration = 0x30000;

    QueuedThreadPool tp(numThreads);
    vector<RegionPropertiesList> results;

    cout << "Initial number of regions: " << initialNumberOfRegions
         << endl;
    cout << "Sample: \n" << toBeProcessed.back() << endl;
    cout << "Waiting for input: " << endl;
    cin >> bruh;
    // In batches of snapshots, will check each for changes.
    // findPlayerName(maxSnapshotsPerIteration, toBeProcessed, tp);
    findPlayerPosition(maxSnapshotsPerIteration, toBeProcessed, tp);
}
