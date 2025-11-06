#include "gui.hpp"
#include <array>
#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_sdl3.h"
#include "core.hpp"
#include "imgui.h"
#include <chrono>
#include <optional>

void AnalysisMenu::draw() {
    if (this->enabled)
    {
        std::string menuLabel = "Analysis Menu: " + std::to_string(this->pid);
        ImGui::Begin(menuLabel.c_str(), &(this->enabled));
        ImGui::End();
    }
}

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
    this->window = SDL_CreateWindow("Dear ImGui SDL3+OpenGL3 example",
                                    (int)(1280 * this->mainScale),
                                    (int)(800 * this->mainScale),
                                    this->windowFlags);
    if (this->window == nullptr)
    {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        validState = false;
        return;
    }
    this->glContext = SDL_GL_CreateContext(this->window);
    if (this->glContext == nullptr)
    {
        printf("Error: SDL_GL_CreateContext(): %s\n", SDL_GetError());
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
    ImGui::CreateContext();
    this->io = ImGui::GetIO();
    (void)this->io;
    this->io.ConfigFlags |=
        ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    this->io.ConfigFlags |=
        ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
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
    this->showDemoWindow    = false;
    this->showAnotherWindow = false;
    this->showMemAnalWindow = true;
    this->bgColor           = ImVec4(0.2f, 0.2f, 0.2f, 1.00f);
    validState              = true;

    if (opt_pid.has_value()) {
        pid_t pid = opt_pid.value();
        opt_demoTestState.emplace(pid);
    }
}

void GuiState::endFrame()
{
    // Rendering
    ImGui::Render();
    glViewport(0, 0, (int)this->io.DisplaySize.x,
               (int)this->io.DisplaySize.y);
    glClearColor(this->bgColor.x * this->bgColor.w,
                 this->bgColor.y * this->bgColor.w,
                 this->bgColor.z * this->bgColor.w, this->bgColor.w);
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
    static char inputBuffer[1024] = {0};
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("Main Menu"))
        {
            ImGui::MenuItem("Show Main Window", "Ctrl + m",
                            &this->showMemAnalWindow);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Demo stuff"))
        {
            ImGui::MenuItem("Show Demo", NULL, &this->showDemoWindow);
            ImGui::MenuItem("Show extra window", NULL,
                            &this->showAnotherWindow);
            if (opt_demoTestState)
            {
                DemoTestState &ds = opt_demoTestState.value();
                ImGui::MenuItem("Show refreshable snapshot demo", NULL, &ds.rsms.enabled);
                ImGui::MenuItem("Show changing refreshable snapshot demo", NULL, &ds.crsms.enabled);
                ImGui::MenuItem("Show AnalysisMenu Demo", NULL, &ds.demoAnalysisMenu.enabled);
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
    if (opt_demoTestState) {
        opt_demoTestState.value().draw();
    }

    if (this->showMemAnalWindow)
    {
        ImGui::Begin("Mem-Anal!", &this->showMemAnalWindow,
                     ImGuiWindowFlags_MenuBar);
        if (ImGui::Button("Open PID"))
            ImGui::OpenPopup("Enter PID");
        if (ImGui::TreeNode("Currently open PIDs"))
        {
            ImGui::BeginChild("##PID Menu", ImVec2(-FLT_MIN, ImGui::GetFontSize() * 10));
            for (auto &[pid, menu] : this->analysisMenuList)
            {
                bool *enabled = &(menu.enabled);
                ImGui::MenuItem(std::to_string(pid).c_str(), NULL, enabled);

                menu.draw();
                
            }
            ImGui::EndChild();
            ImGui::TreePop();
        }

        if (ImGui::BeginPopupModal("Enter PID", NULL,
                                   ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Enter PID (Empty to cancel):");
            if (ImGui::IsWindowAppearing())
            {
                ImGui::SetKeyboardFocusHere();
            }
            if (ImGui::InputText(
                    "PID##Enter PID InputBox", inputBuffer, 1024,
                    ImGuiInputTextFlags_EnterReturnsTrue |
                        ImGuiInputTextFlags_CharsDecimal))
            {
                if (inputBuffer[0] != '\0'){
                    pid_t pid = std::stoi(inputBuffer);
                    this->analysisMenuList.insert({pid, {pid}});
                    memset(inputBuffer, 0, 1024);
                }
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        ImGui::End();
    }

    if (this->showAnotherWindow)
    {
        ImGui::Begin("Another Window");
        ImGui::Text("This is another window!");
        ImGui::End();
    }
    if (this->showDemoWindow)
    {
        ImGui::ShowDemoWindow(&this->showDemoWindow);
    }
    if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_M,
                        ImGuiInputFlags_RouteAlways))
    {
        this->showMemAnalWindow = !this->showMemAnalWindow;
    }
}

void RefreshableSnapshotMenu::modifyPropertiesSubMenu()
{
    auto rs_sptr = m_rs_wptr.lock();
    if (!this->enabled || !rs_sptr)
    {
        this->enabled = false;
        return;
    }
    const static uintptr_t minSize = 0x8;
    auto&                  mrp     = rs_sptr->mrp;
    ImGui::DragScalar("Relative Region Start", ImGuiDataType_U64,
                      &mrp.relativeRegionStart, 8.f, 0,
                      &mrp.parentRegionSize - minSize, "%#lx",
                      ImGuiSliderFlags_AlwaysClamp);
    uintptr_t maxSize =
        mrp.parentRegionSize - mrp.relativeRegionStart;
    if (mrp.relativeRegionSize + mrp.relativeRegionStart >
        mrp.parentRegionSize)
    {
        mrp.relativeRegionSize =
            mrp.parentRegionSize - mrp.relativeRegionStart;
    }
    ImGui::DragScalar("Relative Region Size", ImGuiDataType_U64,
                      &mrp.relativeRegionSize, 8.f, &minSize,
                      &maxSize, "%#lx",
                      ImGuiSliderFlags_AlwaysClamp |
                          ImGuiSliderFlags_NoSpeedTweaks);
}

// TODO: Disable selection of a datatype if the size is too big for the snapshot.
// BUG TODO: Bounds checking. I am definitely not doing bounds checking rn.
// cannot be fucking assed.
void RefreshableSnapshotMenu::draw()
{
    using namespace std::string_literals;
    using namespace std::chrono;
    auto rs_sptr = m_rs_wptr.lock();
    if (!this->enabled || !rs_sptr)
    {
        this->enabled = false;
        return;
    }
    // Used for keeping track of the current index in the snapshot for
    // gui creation.
    uintptr_t curOffset = 0;

    // This seems very サス.
    if (!ImGui::Begin(("Refreshable Snapshot Window: "s +
                       rs_sptr->mrp.regionName)
                          .c_str(),
                      &this->enabled))
    {
        ImGui::End();
    }
    if (ImGui::TreeNode("Region Properties"))
    {
        ImGui::Text("%s",
                    rs_sptr->snap().regionProperties.toStr().c_str());
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Modify Properties"))
    {
        modifyPropertiesSubMenu();
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Refreshing settings"))
    {
        ImGui::Checkbox("Toggle auto refreshing", &this->autoRefresh);
        if (!this->autoRefresh)
        {
            if (ImGui::Button("Refresh"))
            {
                this->refreshSnapshot();
            }
        }
        else
        {
            ImGui::DragScalar("Refresh Rate (ms)", ImGuiDataType_U32,
                              &this->refreshRateMS, 2.5f, 0, NULL,
                              "%u ms");
            if (steady_clock::now().time_since_epoch() -
                    this->lastRefreshTime >
                milliseconds(this->refreshRateMS))
            {
                this->refreshSnapshot();
                this->lastRefreshTime =
                    steady_clock::now().time_since_epoch();
            }
        }
        ImGui::TreePop();
    }
    ImGui::Text("Note that memory is Little Endian");

    if (ImGui::BeginCombo("datatype chooser",
                          dataTypeTable[this->curType], 0))
    {
        for (size_t n = 0; n < IM_ARRAYSIZE(dataTypeTable); n++)
        {
            const bool isSelected = (this->curType == n);
            if (ImGui::Selectable(dataTypeTable[n], isSelected))
                this->curType = static_cast<e_dataTypes>(n);

            if (isSelected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    const size_t numCols =
        (rs_sptr->snap().size() > 8) ? 16 : rs_sptr->snap().size();
    const size_t selSize = dataSizeTable[this->curType];
    if (!ImGui::BeginChild(
            "##SnapView",
            ImVec2(-FLT_MIN, ImGui::GetFontSize() * 40)))
    {
        ImGui::EndChild();
        ImGui::End();
    }

    auto tableFlags =
        ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit;

    ImGuiMultiSelectFlags flags =
        ImGuiMultiSelectFlags_ClearOnEscape |
        ImGuiMultiSelectFlags_BoxSelect1d;
    ImGuiMultiSelectIO* ms_io = ImGui::BeginMultiSelect(
        flags, this->selection.Size, rs_sptr->snap().size());
    this->selection.ApplyRequests(ms_io);

    this->changeRequest =
        ImGui::Shortcut(ImGuiKey_Space) && (this->selection.Size > 0);

    if (!ImGui::BeginTable("Snapshot Table", 2, tableFlags))
    {
        ImGui::EndTable();
        ImGui::EndChild();
        ImGui::End();
    }
    static const float singleByteWidth =
        ImGui::CalcTextSize("00").x + 5;
    static const float singleByteHeight = ImGui::CalcTextSize("00").y;

    curOffset            = 0;
    size_t textCursorPos = 1;
    ImGui::TableNextColumn();
    char labelBuffer[32] = {0};
    while (curOffset < rs_sptr->snap().size())
    {
        const auto initialOff = curOffset;
        auto       curType    = this->types[curOffset];
        auto       curSize    = dataSizeTable[curType];

        float      selWidth  = singleByteWidth * selSize;
        float      cellWidth = singleByteWidth * curSize;
        if (selSize < curSize)
        {
            selWidth = cellWidth;
        }

        ImGui::SameLine(textCursorPos);
        sprintf(labelBuffer, "##%lu", curOffset);
        ImGui::SetNextItemSelectionUserData(curOffset);
        bool selected = this->selection.Contains(curOffset);
        ImGui::Selectable(labelBuffer, selected,
                          ImGuiSelectableFlags_None,
                          {selWidth - 5, singleByteHeight});

        size_t selOffset = 0;
        while (selOffset < selWidth)
        {
            auto  curType   = this->types[curOffset];
            auto  curSize   = dataSizeTable[curType];
            float cellWidth = singleByteWidth * curSize;
            ImGui::SameLine(textCursorPos);
            uint64_t val;
            memcpy(&val, rs_sptr->snap().data() + curOffset, curSize);
            ImGui::Text(dataConvTable[curType], val);
            textCursorPos += cellWidth;
            selOffset += cellWidth;
            curOffset += curSize;
        }

        if (this->changeRequest && selected)
        {
            Log(Debug, "Selection ID: " << initialOff);
            Log(Debug, "CurSize: " << curSize);
            Log(Debug, "SelSize: " << selSize);
            // When resetting, take the larger of the 2 sizes;
            size_t resetSize = curSize > selSize ? curSize : selSize;
            if (this->types[initialOff] != HEX)
            {
                Log(Debug, "type unknown -> hex");
                memset(this->types.data() + initialOff, HEX,
                       resetSize);
            }
            else
            {
                Log(Debug, "type hex -> unknown");
                memset(this->types.data() + initialOff, this->curType,
                       selSize);
            }
        }

        // // Print ascii conversion.
        if (curOffset % numCols == 0 ||
            curOffset == rs_sptr->snap().size())
        {
            ImGui::TableNextColumn();
            char strVer[17] = {0};
            // Bounds check for bottom row lacking enough data.
            size_t boundsOffset = curOffset % numCols;
            if (boundsOffset == 0)
                boundsOffset = numCols;
            memcpy(strVer,
                   rs_sptr->snap().data() + curOffset - boundsOffset,
                   boundsOffset);
            for (size_t j = 0; j < numCols; j++)
            {
                if (strVer[j] - 31 > 0)
                    continue;
                strVer[j] = '.';
            }

            ImGui::Text("%s", strVer);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            textCursorPos = 1;
        }
    }
    ms_io = ImGui::EndMultiSelect();
    this->selection.ApplyRequests(ms_io);
    ImGui::EndTable();
    ImGui::EndChild();
    ImGui::End();
}
