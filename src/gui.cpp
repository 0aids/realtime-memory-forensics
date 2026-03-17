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
        if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
        {
            printf("Error: SDL_Init(): %s\n", SDL_GetError());
            validState = false;
            return;
        }

        // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
        this->glslVersion = "#version 100";
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                            SDL_GL_CONTEXT_PROFILE_ES);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(IMGUI_IMPL_OPENGL_ES3)
        this->glslVersion = "#version 300 es";
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                            SDL_GL_CONTEXT_PROFILE_ES);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(__APPLE__)
        this->glslVersion = "#version 150";
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS,
                            SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                            SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
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

        // Setup scaling
        ImGuiStyle& style = ImGui::GetStyle();
        style.ScaleAllSizes(this->mainScale);
        style.FontScaleDpi = this->mainScale;

        // Setup Platform/Renderer backends
        ImGui_ImplSDL3_InitForOpenGL(this->window, this->glContext);
        ImGui_ImplOpenGL3_Init(this->glslVersion.c_str());

        // Our state
        this->showDemoWindow    = true;
        this->showAnotherWindow = false;
        this->showMemAnalWindow = true;
        this->bgColor           = ImVec4(0.2f, 0.2f, 0.2f, 1.00f);
        validState              = true;

        // Initialize managers
        processManager = std::make_unique<ProcessManager>();
        scanManager  = std::make_unique<ScanManager>(*processManager);
        graphManager = std::make_unique<GraphManager>();

        if (opt_pid.has_value())
        {
            processManager->attachToProcess(opt_pid.value());
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
        viewport    = ImGui::GetMainViewport();

        // Create settings
        if (ImGui::DockBuilderGetNode(dockspaceId) == nullptr)
        {
            ImGui::DockBuilderAddNode(dockspaceId,
                                      ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(dockspaceId,
                                          viewport->Size);
            ImGuiID dock_id_left = 0;
            ImGuiID dock_id_main = dockspaceId;
            ImGui::DockBuilderSplitNode(dock_id_main, ImGuiDir_Left,
                                        0.20f, &dock_id_left,
                                        &dock_id_main);
            ImGuiID dock_id_left_top    = 0;
            ImGuiID dock_id_left_bottom = 0;
            ImGui::DockBuilderSplitNode(dock_id_left, ImGuiDir_Up,
                                        0.50f, &dock_id_left_top,
                                        &dock_id_left_bottom);
            ImGui::DockBuilderDockWindow("Game", dock_id_main);
            ImGui::DockBuilderDockWindow("Properties",
                                         dock_id_left_top);
            ImGui::DockBuilderDockWindow("Scene",
                                         dock_id_left_bottom);
            ImGui::DockBuilderFinish(dockspaceId);
        }
        ImGui::DockSpaceOverViewport(
            dockspaceId, viewport,
            ImGuiDockNodeFlags_PassthruCentralNode);
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("View"))
            {
                ImGui::MenuItem("Process Attachment", NULL,
                                &this->showProcessWindow);
                ImGui::MenuItem("Memory Regions", NULL,
                                &this->showMemoryRegionsWindow);
                ImGui::MenuItem("Scan", NULL, &this->showScanWindow);
                ImGui::MenuItem("Node Viewer", NULL,
                                &this->showNodeViewer);
                ImGui::Separator();
                ImGui::MenuItem("Demo Window", NULL,
                                &this->showDemoWindow);
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        // Draw managers
        if (showProcessWindow)
        {
            processManager->draw();
        }

        // Memory Regions Window (requires interaction with GraphManager)
        if (showMemoryRegionsWindow &&
            processManager->getTargetPid().has_value())
        {
            if (ImGui::Begin("Memory Regions",
                             &showMemoryRegionsWindow))
            {
                const auto& currentRegions =
                    processManager->getCurrentRegions();
                ImGui::Text("PID %d: %zu regions",
                            processManager->getTargetPid().value(),
                            currentRegions.size());
                ImGui::SameLine();
                if (ImGui::Button("Refresh##regions"))
                {
                    processManager->attachToProcess(
                        processManager->getTargetPid().value());
                }

                static char filterBuf[128] = "";
                ImGui::InputText("Filter", filterBuf,
                                 sizeof(filterBuf));
                std::string regionFilter = filterBuf;

                float       tableHeight =
                    ImGui::GetContentRegionAvail().y - 60;
                if (tableHeight < 100)
                    tableHeight = 100;

                static int selectedRegionIndex = -1;
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
                        graphManager->addRegionFromProcess(mrp);
                    }
                }
            }
            ImGui::End();
        }

        if (showScanWindow)
        {
            scanManager->draw(
                [this]()
                {
                    // Handle "Add to Graph" from scan results
                    if (!scanManager->getScanResults().empty() &&
                        ImGui::Button("Add All to Graph"))
                    {
                        for (const auto& result :
                             scanManager->getScanResults())
                        {
                            graphManager->addRegionFromScan(
                                result.mrp,
                                std::format("Scan: {}",
                                            result.valueStr));
                        }
                    }
                });
        }

        if (showNodeViewer)
        {
            graphManager->draw();
        }

        if (this->showDemoWindow)
        {
            ImGui::ShowDemoWindow(&this->showDemoWindow);
        }
    }

}
