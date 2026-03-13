#include "imnodes.h"
#include "utils.hpp"
#include "operations.hpp"
#include <memory>
#include <filesystem>
#include <fstream>
#include <magic_enum/magic_enum.hpp>
#ifdef IMGUI_DISABLE_DEMO_WINDOWS
#undef IMGUI_DISABLE_DEMO_WINDOWS
#endif
#include <vector>
#include <stack>
#include <algorithm>
#include "gui.hpp"
#include "imgui.h"
#include <backends/imgui_impl_opengl3.h>
#include <backends/imgui_impl_sdl3.h>
#include <optional>

namespace rmf::gui
{

    GuiState::GuiState(std::optional<pid_t> opt_pid)
    {
        // Setup SDL
        // [If using SDL_MAIN_USE_CALLBACKS: all code below until the main loop starts would likely be your SDL_AppInit() function]
        if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
        {
            printf("Error: SDL_Init(): %s\n", SDL_GetError());
            validState = false;
            return;
        }

        // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
        // GL ES 2.0 + GLSL 100 (WebGL 1.0)
        this->glslVersion = "#version 100";
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                            SDL_GL_CONTEXT_PROFILE_ES);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(IMGUI_IMPL_OPENGL_ES3)
        // GL ES 3.0 + GLSL 300 es (WebGL 2.0)
        this->glslVersion = "#version 300 es";
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                            SDL_GL_CONTEXT_PROFILE_ES);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(__APPLE__)
        // GL 3.2 Core + GLSL 150
        this->glslVersion = "#version 150";
        SDL_GL_SetAttribute(
            SDL_GL_CONTEXT_FLAGS,
            SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                            SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
        // GL 3.0 + GLSL 130
        this->glslVersion = "#version 130";
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                            SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

        // Create window with graphics context
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
        this->mainScale =
            SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
        this->windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
            SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
        this->window = SDL_CreateWindow(
            "Dear ImGui SDL3+OpenGL3 example",
            (int)(1280 * this->mainScale),
            (int)(800 * this->mainScale), this->windowFlags);
        if (this->window == nullptr)
        {
            printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
            validState = false;
            return;
        }
        this->glContext = SDL_GL_CreateContext(this->window);
        if (this->glContext == nullptr)
        {
            printf("Error: SDL_GL_CreateContext(): %s\n",
                   SDL_GetError());
            validState = false;
            return;
        }

        SDL_GL_MakeCurrent(this->window, this->glContext);
        SDL_GL_SetSwapInterval(1); // Enable vsync
        SDL_SetWindowPosition(this->window, SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED);
        SDL_ShowWindow(this->window);

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        auto context = ImGui::CreateContext();
        ImGui::SetCurrentContext(context);
        ImNodes::CreateContext();
        this->io = ImGui::GetIO();
        (void)this->io;
        this->io.ConfigFlags |=
            ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
        this->io.ConfigFlags |=
            ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        ImNodes::StyleColorsDark();
        //ImGui::StyleColorsLight();

        // Setup scaling
        ImGuiStyle& style = ImGui::GetStyle();
        style.ScaleAllSizes(
            this->mainScale); // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
        style.FontScaleDpi =
            this->mainScale; // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this unnecessary. We leave both here for documentation purpose)

        // Setup Platform/Renderer backends
        ImGui_ImplSDL3_InitForOpenGL(this->window, this->glContext);
        ImGui_ImplOpenGL3_Init(this->glslVersion.c_str());

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
        this->showDemoWindow    = true;
        this->showAnotherWindow = false;
        this->showMemAnalWindow = true;
        this->bgColor           = ImVec4(0.2f, 0.2f, 0.2f, 1.00f);
        validState              = true;

        // Analysis state initialization
        this->analyzerThreadCount = 6;
        this->analyzer =
            std::make_unique<rmf::Analyzer>(analyzerThreadCount);
        this->selectedRegionIndex = -1;
        refreshProcessList();

        if (opt_pid.has_value())
        {
            attachToProcess(opt_pid.value());
        }

        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    }

    GuiState::~GuiState()
    {
        // Cleanup
        // [If using SDL_MAIN_USE_CALLBACKS: all code below would likely be your SDL_AppQuit() function]
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImNodes::DestroyContext();
        ImGui::DestroyContext();

        SDL_GL_DestroyContext(glContext);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }

    void GuiState::endFrame()
    {
        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)this->io.DisplaySize.x,
                   (int)this->io.DisplaySize.y);
        glClearColor(this->bgColor.x * this->bgColor.w,
                     this->bgColor.y * this->bgColor.w,
                     this->bgColor.z * this->bgColor.w,
                     this->bgColor.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(this->window);
    }

    void GuiState::startFrame()
    {
        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
    }

    void GuiState::draw()
    {
        dockspaceId = ImGui::GetID("Dockspace");
        viewport = ImGui::GetMainViewport();
        
        // Create settings
        if (ImGui::DockBuilderGetNode(dockspaceId) == nullptr)
        {
            ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(dockspaceId, viewport->Size);
            ImGuiID dock_id_left = 0;
            ImGuiID dock_id_main = dockspaceId;
            ImGui::DockBuilderSplitNode(dock_id_main, ImGuiDir_Left, 0.20f, &dock_id_left, &dock_id_main);
            ImGuiID dock_id_left_top = 0;
            ImGuiID dock_id_left_bottom = 0;
            ImGui::DockBuilderSplitNode(dock_id_left, ImGuiDir_Up, 0.50f, &dock_id_left_top, &dock_id_left_bottom);
            ImGui::DockBuilderDockWindow("Game", dock_id_main);
            ImGui::DockBuilderDockWindow("Properties", dock_id_left_top);
            ImGui::DockBuilderDockWindow("Scene", dock_id_left_bottom);
            ImGui::DockBuilderFinish(dockspaceId);
        }
        ImGui::DockSpaceOverViewport(dockspaceId, viewport, ImGuiDockNodeFlags_PassthruCentralNode);
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("View"))
            {
                ImGui::MenuItem("Process Attachment", NULL,
                                &this->showProcessWindow);
                ImGui::MenuItem("Memory Regions", NULL,
                                &this->showMemoryRegionsWindow);
                ImGui::MenuItem("Scan", NULL, &this->showScanWindow);
                ImGui::Separator();
                ImGui::MenuItem("Demo Window", NULL,
                                &this->showDemoWindow);
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        // === Process Attachment Window ===
        if (showProcessWindow)
        {
            if (ImGui::Begin("Process Attachment",
                             &showProcessWindow))
            {
                if (targetPid.has_value())
                {
                    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f),
                                       "PID: %d", targetPid.value());
                    ImGui::SameLine();
                    ImGui::Text("Name: %s",
                                targetProcessName.c_str());
                    if (ImGui::Button("Detach"))
                    {
                        detachFromProcess();
                    }
                }
                else
                {
                    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f),
                                       "No process attached");

                    static int manualPid = 0;
                    ImGui::InputInt("PID", &manualPid);
                    ImGui::SameLine();
                    if (ImGui::Button("Attach"))
                    {
                        if (manualPid > 0)
                        {
                            attachToProcess(manualPid);
                            manualPid = 0;
                        }
                    }

                    ImGui::Separator();
                    ImGui::Text("Process Browser:");
                    if (ImGui::Button("Refresh"))
                    {
                        refreshProcessList();
                    }

                    static int selectedProcess = -1;
                    float      listHeight =
                        ImGui::GetTextLineHeightWithSpacing() * 10;
                    if (ImGui::BeginListBox(
                            "Processes",
                            ImVec2(-FLT_MIN, listHeight)))
                    {
                        for (int i = 0;
                             i < static_cast<int>(processList.size());
                             i++)
                        {
                            std::string label =
                                std::to_string(processList[i].first) +
                                " - " + processList[i].second;
                            if (ImGui::Selectable(
                                    label.c_str(),
                                    selectedProcess == i,
                                    ImGuiSelectableFlags_AllowDoubleClick))
                            {
                                selectedProcess = i;
                                if (ImGui::IsMouseDoubleClicked(
                                        ImGuiMouseButton_Left))
                                {
                                    attachToProcess(
                                        processList[i].first);
                                    selectedProcess = -1;
                                }
                            }
                        }
                        ImGui::EndListBox();
                    }

                    if (selectedProcess >= 0 &&
                        selectedProcess <
                            static_cast<int>(processList.size()))
                    {
                        ImGui::SameLine();
                        if (ImGui::Button("Attach##selected"))
                        {
                            attachToProcess(
                                processList[selectedProcess].first);
                            selectedProcess = -1;
                        }
                    }
                }
            }
            ImGui::End();
        }

        // === Memory Regions Window ===
        if (showMemoryRegionsWindow && targetPid.has_value())
        {
            if (ImGui::Begin("Memory Regions",
                             &showMemoryRegionsWindow))
            {
                ImGui::Text("PID %d: %zu regions", targetPid.value(),
                            currentRegions.size());
                ImGui::SameLine();
                if (ImGui::Button("Refresh##regions"))
                {
                    std::string mapsPath = "/proc/" +
                        std::to_string(targetPid.value()) + "/maps";
                    currentRegions = rmf::utils::ParseMaps(mapsPath);
                }

                static char filterBuf[128] = "";
                ImGui::InputText("Filter", filterBuf,
                                 sizeof(filterBuf));
                regionFilter = filterBuf;

                float tableHeight =
                    ImGui::GetContentRegionAvail().y - 60;
                if (tableHeight < 100)
                    tableHeight = tableHeight;

                if (ImGui::BeginTable("Regions", 4,
                                      ImGuiTableFlags_Borders |
                                          ImGuiTableFlags_Resizable |
                                          ImGuiTableFlags_RowBg |
                                          ImGuiTableFlags_ScrollY,
                                      ImVec2(-FLT_MIN, tableHeight)))
                {
                    ImGui::TableSetupColumn("Address");
                    ImGui::TableSetupColumn("Size");
                    ImGui::TableSetupColumn("Perms");
                    ImGui::TableSetupColumn("Name");
                    ImGui::TableHeadersRow();

                    for (int i = 0;
                         i < static_cast<int>(currentRegions.size());
                         i++)
                    {
                        const auto& mrp = currentRegions[i];
                        std::string addrStr =
                            std::format("0x{:x}", mrp.TrueAddress());
                        std::string sizeStr = std::format(
                            "0x{:x}", mrp.relativeRegionSize);
                        std::string permsStr = std::string(
                            magic_enum::enum_flags_name(mrp.perms));
                        std::string nameStr = *mrp.regionName_sp;

                        if (!regionFilter.empty())
                        {
                            bool matches =
                                addrStr.find(regionFilter) !=
                                    std::string::npos ||
                                sizeStr.find(regionFilter) !=
                                    std::string::npos ||
                                permsStr.find(regionFilter) !=
                                    std::string::npos ||
                                nameStr.find(regionFilter) !=
                                    std::string::npos;
                            if (!matches)
                                continue;
                        }

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        bool isSelected = (selectedRegionIndex == i);
                        if (ImGui::Selectable(
                                addrStr.c_str(), isSelected,
                                ImGuiSelectableFlags_SpanAllColumns))
                        {
                            selectedRegionIndex = i;
                        }
                        ImGui::TableSetColumnIndex(1);
                        ImGui::TextUnformatted(sizeStr.c_str());
                        ImGui::TableSetColumnIndex(2);
                        ImGui::TextUnformatted(permsStr.c_str());
                        ImGui::TableSetColumnIndex(3);
                        ImGui::TextUnformatted(nameStr.c_str());
                    }
                    ImGui::EndTable();
                }

                if (selectedRegionIndex >= 0 &&
                    selectedRegionIndex <
                        static_cast<int>(currentRegions.size()))
                {
                    ImGui::Separator();
                    const auto& mrp =
                        currentRegions[selectedRegionIndex];
                    ImGui::Text("Selected: 0x%lx (0x%lx)",
                                mrp.relativeRegionAddress,
                                mrp.relativeRegionSize);
                    if (ImGui::Button("Add to Graph"))
                    {
                        rmf::graph::MemoryRegionData newData;
                        newData.name = *mrp.regionName_sp;
                        newData.mrp  = mrp;
                        mgViewerTest.sp_mg->RegionAdd(newData);
                    }
                }
            }
            ImGui::End();
        }

        // === Scan Window ===
        if (showScanWindow && targetPid.has_value())
        {
            if (ImGui::Begin("Scan", &showScanWindow))
            {
                // Operation selector
                const char* ops[] = {"Exact Value", "Changed",
                                     "Unchanged", "String",
                                     "Pointer to Region"};
                int currentOp     = static_cast<int>(scanOperation);
                if (ImGui::Combo("Operation", &currentOp, ops,
                                 IM_ARRAYSIZE(ops)))
                {
                    scanOperation =
                        static_cast<ScanOperation>(currentOp);
                }

                // Value type selector (only for numeric operations)
                if (scanOperation == ScanOperation::ExactValue ||
                    scanOperation == ScanOperation::Changed ||
                    scanOperation == ScanOperation::Unchanged)
                {
                    const char* types[] = {"i8",  "i16", "i32", "i64",
                                           "u8",  "u16", "u32", "u64",
                                           "f32", "f64"};
                    int currentType = static_cast<int>(scanValueType);
                    if (ImGui::Combo("Type", &currentType, types,
                                     IM_ARRAYSIZE(types)))
                    {
                        scanValueType =
                            static_cast<ScanValueType>(currentType);
                    }
                }

                // Value input
                ImGui::Checkbox("Hex", &scanHexInput);
                ImGui::SameLine();

                const char* hint =
                    scanHexInput ? "0xDEADBEEF" : "12345";
                ImGui::InputText(
                    "Value", scanValueBuf, sizeof(scanValueBuf),
                    ImGuiInputTextFlags_CharsHexadecimal);

                // Scan button
                if (!isScanning &&
                    ImGui::Button("Run Scan", ImVec2(100, 30)))
                {
                    runScan();
                }

                if (isScanning)
                {
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f),
                                       "Scanning...");
                }

                ImGui::Separator();

                // Results
                ImGui::Text("Results: %zu (%.2f ms)",
                            scanResults.size(),
                            std::chrono::duration<double, std::milli>(
                                scanDuration)
                                .count());

                if (!scanResults.empty())
                {
                    if (ImGui::Button("Add All to Graph"))
                    {
                        for (const auto& result : scanResults)
                        {
                            rmf::graph::MemoryRegionData newData;
                            newData.name = std::format(
                                "Scan: {}", result.valueStr);
                            newData.mrp = result.mrp;
                            mgViewerTest.sp_mg->RegionAdd(newData);
                        }
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Clear"))
                    {
                        clearScanResults();
                    }
                }

                float tableHeight =
                    ImGui::GetContentRegionAvail().y - 60;
                if (tableHeight < 100)
                    tableHeight = 100;

                if (ImGui::BeginTable("ScanResults", 3,
                                      ImGuiTableFlags_Borders |
                                          ImGuiTableFlags_Resizable |
                                          ImGuiTableFlags_RowBg |
                                          ImGuiTableFlags_ScrollY,
                                      ImVec2(-FLT_MIN, tableHeight)))
                {
                    ImGui::TableSetupColumn("Address");
                    ImGui::TableSetupColumn("Value");
                    ImGui::TableSetupColumn("Region");
                    ImGui::TableHeadersRow();

                    for (int i = 0;
                         i < static_cast<int>(scanResults.size());
                         i++)
                    {
                        const auto& result = scanResults[i];
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        bool isSelected = (selectedResultIndex == i);
                        std::string addrStr =
                            std::format("0x{:x}", result.address);
                        if (ImGui::Selectable(
                                addrStr.c_str(), isSelected,
                                ImGuiSelectableFlags_SpanAllColumns))
                        {
                            selectedResultIndex = i;
                        }
                        ImGui::TableSetColumnIndex(1);
                        ImGui::TextUnformatted(
                            result.valueStr.c_str());
                        ImGui::TableSetColumnIndex(2);
                        ImGui::TextUnformatted(
                            result.mrp.regionName_sp->c_str());
                    }
                    ImGui::EndTable();
                }

                if (selectedResultIndex >= 0 &&
                    selectedResultIndex <
                        static_cast<int>(scanResults.size()))
                {
                    ImGui::Separator();
                    const auto& result =
                        scanResults[selectedResultIndex];
                    ImGui::Text("Selected: 0x%lx", result.address);
                    if (ImGui::Button("Add to Graph"))
                    {
                        rmf::graph::MemoryRegionData newData;
                        newData.name =
                            std::format("Scan: {}", result.valueStr);
                        newData.mrp = result.mrp;
                        mgViewerTest.sp_mg->RegionAdd(newData);
                    }
                }
            }
            ImGui::End();
        }

        // === Main: Graph Editor ===

        // This is here on purpose.
        ImGui::Begin("Node Viewer");
        mgViewerTest.draw();
        ImGui::End();

        if (this->showDemoWindow)
        {
            ImGui::ShowDemoWindow(&this->showDemoWindow);
        }
    }

    void GuiState::refreshProcessList()
    {
        processList.clear();
        const std::string procPath = "/proc";

        for (const auto& entry :
             std::filesystem::directory_iterator(procPath))
        {
            std::string name = entry.path().filename();
            if (std::all_of(name.begin(), name.end(), ::isdigit))
            {
                pid_t       pid = static_cast<pid_t>(std::stoi(name));
                std::string commPath =
                    entry.path().string() + "/comm";
                std::string   procName = name;

                std::ifstream commFile(commPath);
                if (commFile.is_open())
                {
                    std::getline(commFile, procName);
                    commFile.close();
                }

                processList.emplace_back(pid, procName);
            }
        }

        std::sort(processList.begin(), processList.end(),
                  [](const auto& a, const auto& b)
                  { return a.second < b.second; });
    }

    void GuiState::attachToProcess(pid_t pid)
    {
        std::string commPath =
            "/proc/" + std::to_string(pid) + "/comm";
        std::ifstream commFile(commPath);
        if (commFile.is_open())
        {
            std::getline(commFile, targetProcessName);
            commFile.close();
        }
        else
        {
            targetProcessName = "Unknown";
        }

        targetPid = pid;

        std::string mapsPath =
            "/proc/" + std::to_string(pid) + "/maps";
        currentRegions      = rmf::utils::ParseMaps(mapsPath);
        selectedRegionIndex = -1;
    }

    void GuiState::detachFromProcess()
    {
        targetPid.reset();
        targetProcessName.clear();
        currentRegions.clear();
        selectedRegionIndex = -1;
        clearScanResults();
    }

    void GuiState::clearScanResults()
    {
        scanResults.clear();
        selectedResultIndex = -1;
    }

    void GuiState::runScan()
    {
        if (!targetPid.has_value() || currentRegions.empty())
            return;

        clearScanResults();
        isScanning    = true;
        scanStartTime = std::chrono::steady_clock::now();

        pid_t pid = targetPid.value();

        // Filter readable regions for scanning
        auto readableRegions = currentRegions.FilterHasPerms("r");

        // Take snapshots - wrap in lambda to pass pid
        auto makeSnap = [&](const types::MemoryRegionProperties& mrp)
        { return types::MemorySnapshot::Make(mrp, pid); };

        auto snaps = analyzer->Execute(makeSnap, readableRegions);

        std::vector<types::MemoryRegionPropertiesVec> results;

        // Parse the input value
        std::string valueStr(scanValueBuf);

        try
        {
            switch (scanOperation)
            {
                case ScanOperation::ExactValue:
                {
                    if (scanValueType == ScanValueType::i32)
                    {
                        int32_t val = static_cast<int32_t>(
                            std::stoll(valueStr, nullptr,
                                       scanHexInput ? 16 : 10));
                        auto scanOp =
                            [&](const types::MemorySnapshot& snap)
                        {
                            return rmf::op::findNumeralExact<int32_t>(
                                snap, val);
                        };
                        results = analyzer->Execute(scanOp, snaps);
                    }
                    else if (scanValueType == ScanValueType::i64)
                    {
                        int64_t val =
                            std::stoll(valueStr, nullptr,
                                       scanHexInput ? 16 : 10);
                        auto scanOp =
                            [&](const types::MemorySnapshot& snap)
                        {
                            return rmf::op::findNumeralExact<int64_t>(
                                snap, val);
                        };
                        results = analyzer->Execute(scanOp, snaps);
                    }
                    else if (scanValueType == ScanValueType::u32)
                    {
                        uint32_t val = static_cast<uint32_t>(
                            std::stoull(valueStr, nullptr,
                                        scanHexInput ? 16 : 10));
                        auto scanOp =
                            [&](const types::MemorySnapshot& snap)
                        {
                            return rmf::op::findNumeralExact<
                                uint32_t>(snap, val);
                        };
                        results = analyzer->Execute(scanOp, snaps);
                    }
                    else if (scanValueType == ScanValueType::u64)
                    {
                        uint64_t val =
                            std::stoull(valueStr, nullptr,
                                        scanHexInput ? 16 : 10);
                        auto scanOp =
                            [&](const types::MemorySnapshot& snap)
                        {
                            return rmf::op::findNumeralExact<
                                uint64_t>(snap, val);
                        };
                        results = analyzer->Execute(scanOp, snaps);
                    }
                    break;
                }
                case ScanOperation::String:
                {
                    std::string_view sv = valueStr;
                    auto             scanOp =
                        [&](const types::MemorySnapshot& snap)
                    { return rmf::op::findString(snap, sv); };
                    results = analyzer->Execute(scanOp, snaps);
                    break;
                }
                default: break;
            }
        }
        catch (const std::exception& e)
        {
            rmf_Log(rmf_Error, "Scan error: " << e.what());
        }

        // Flatten results and convert to ScanResult
        for (const auto& resultVec : results)
        {
            for (const auto& mrp : resultVec)
            {
                ScanResult sr;
                sr.address  = mrp.TrueAddress();
                sr.valueStr = valueStr;
                sr.mrp      = mrp;
                scanResults.push_back(sr);
            }
        }

        scanDuration =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - scanStartTime);
        isScanning = false;
    }
}
