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

        cout << "Finding within a good range" << endl;
        cout << "enter to cont, q to quit: ";
        cin >> bruh;
        if (bruh == "q") break;
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
                        MemorySnapshot(tasks1[j].result.get(),
                                       currentlyProcessing[j],
                                       chrono::steady_clock::now()
                                           .time_since_epoch()));
                    spansList1.push_back(
                        snapshotsList1.back().asSnapshotSpan());
                }
                // Convert into spans.
                inputs     = consolidateIntoCoreInput({
                        .mrpVec   = currentlyProcessing,
                        .snap1Vec = spansList1,
                });
                auto tasks = createMultipleTasks(
                    findNumericWithinRange<float> , inputs, 59.5f, 62.f);
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
            if (toBeProcessed.size() != last) {
                consecNoChange = 0;
            } else {
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

// Main code
int main(int, char**)
{
    // Setup SDL
    // [If using SDL_MAIN_USE_CALLBACKS: all code below until the main loop starts would likely be your SDL_AppInit() function]
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
    {
        printf("Error: SDL_Init(): %s\n", SDL_GetError());
        return 1;
    }

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100 (WebGL 1.0)
    const char* glsl_version = "#version 100";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(IMGUI_IMPL_OPENGL_ES3)
    // GL ES 3.0 + GLSL 300 es (WebGL 2.0)
    const char* glsl_version = "#version 300 es";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(__APPLE__)
    // GL 3.2 Core + GLSL 150
    const char* glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    SDL_WindowFlags window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    SDL_Window* window = SDL_CreateWindow("Dear ImGui SDL3+OpenGL3 example", (int)(1280 * main_scale), (int)(800 * main_scale), window_flags);
    if (window == nullptr)
    {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return 1;
    }
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (gl_context == nullptr)
    {
        printf("Error: SDL_GL_CreateContext(): %s\n", SDL_GetError());
        return 1;
    }

    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(window);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup scaling
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style.FontScaleDpi = main_scale;        // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this unnecessary. We leave both here for documentation purpose)

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details. If you like the default font but want it to scale better, consider using the 'ProggyVector' from the same author!
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    // - Our Emscripten build process allows embedding fonts to be accessible at runtime from the "fonts/" folder. See Makefile.emscripten for details.
    //style.FontSizeBase = 20.0f;
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf");
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf");
    //IM_ASSERT(font != nullptr);

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
    bool done = false;
#ifdef __EMSCRIPTEN__
    // For an Emscripten build we are disabling file-system access, so let's not attempt to do a fopen() of the imgui.ini file.
    // You may manually call LoadIniSettingsFromMemory() to load settings from your own storage.
    io.IniFilename = nullptr;
    EMSCRIPTEN_MAINLOOP_BEGIN
#else
    while (!done)
#endif
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
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT)
                done = true;
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }

        // [If using SDL_MAIN_USE_CALLBACKS: all code below would likely be your SDL_AppIterate() function]
        if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED)
        {
            SDL_Delay(10);
            continue;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
        {
            static float f = 0.0f;
            static int counter = 0;

            ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

            ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
            ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
            ImGui::Checkbox("Another Window", &show_another_window);

            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
            ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

            if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::End();
        }

        // 3. Show another simple window.
        if (show_another_window)
        {
            ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
            ImGui::Text("Hello from another window!");
            if (ImGui::Button("Close Me"))
                show_another_window = false;
            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_END;
#endif

    // Cleanup
    // [If using SDL_MAIN_USE_CALLBACKS: all code below would likely be your SDL_AppQuit() function]
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DestroyContext(gl_context);
    SDL_DestroyWindow(window);
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
