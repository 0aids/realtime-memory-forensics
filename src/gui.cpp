#include "gui.hpp"
#include <array>
#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_sdl3.h"
#include "core.hpp"
#include "imgui.h"
#include <chrono>

bool initGui(GuiState& gs)
{
    // Setup SDL
    // [If using SDL_MAIN_USE_CALLBACKS: all code below until the main loop starts would likely be your SDL_AppInit() function]
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
    {
        printf("Error: SDL_Init(): %s\n", SDL_GetError());
        return false;
    }

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100 (WebGL 1.0)
    gs.glslVersion = "#version 100";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(IMGUI_IMPL_OPENGL_ES3)
    // GL ES 3.0 + GLSL 300 es (WebGL 2.0)
    gs.glslVersion = "#version 300 es";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(__APPLE__)
    // GL 3.2 Core + GLSL 150
    gs.glslVersion = "#version 150";
    SDL_GL_SetAttribute(
        SDL_GL_CONTEXT_FLAGS,
        SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    // GL 3.0 + GLSL 130
    gs.glslVersion = "#version 130";
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
    gs.mainScale =
        SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    gs.windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
        SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    gs.window = SDL_CreateWindow(
        "Dear ImGui SDL3+OpenGL3 example", (int)(1280 * gs.mainScale),
        (int)(800 * gs.mainScale), gs.windowFlags);
    if (gs.window == nullptr)
    {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return false;
    }
    gs.glContext = SDL_GL_CreateContext(gs.window);
    if (gs.glContext == nullptr)
    {
        printf("Error: SDL_GL_CreateContext(): %s\n", SDL_GetError());
        return false;
    }

    SDL_GL_MakeCurrent(gs.window, gs.glContext);
    SDL_GL_SetSwapInterval(1); // Enable vsync
    SDL_SetWindowPosition(gs.window, SDL_WINDOWPOS_CENTERED,
                          SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(gs.window);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    gs._io = ImGui::GetIO();
    (void)gs._io;
    gs.io().ConfigFlags |=
        ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    gs.io().ConfigFlags |=
        ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup scaling
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(
        gs.mainScale); // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style.FontScaleDpi =
        gs.mainScale; // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this unnecessary. We leave both here for documentation purpose)

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForOpenGL(gs.window, gs.glContext);
    ImGui_ImplOpenGL3_Init(gs.glslVersion.c_str());

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
    gs.showDemoWindow    = true;
    gs.showAnotherWindow = false;
    gs.bgColor           = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    return true;
}

void endGuiFrame(GuiState& gs)
{
    // Rendering
    ImGui::Render();
    glViewport(0, 0, (int)gs.io().DisplaySize.x,
               (int)gs.io().DisplaySize.y);
    glClearColor(gs.bgColor.x * gs.bgColor.w,
                 gs.bgColor.y * gs.bgColor.w,
                 gs.bgColor.z * gs.bgColor.w, gs.bgColor.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(gs.window);
}

void initGuiFrame()
{
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

void demoWindows(GuiState& gs)
{
    if (gs.showDemoWindow)
    {
        ImGui::ShowDemoWindow(&gs.showDemoWindow);
    }
    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
    {
        static float f       = 0.0f;
        static int   counter = 0;

        ImGui::Begin(
            "Hello, world!"); // Create a window called "Hello, world!" and append into it.

        ImGui::Text(
            "This is some useful text."); // Display some text (you can use a format strings too)
        ImGui::Checkbox(
            "Demo Window",
            &gs.showDemoWindow); // Edit bools storing our window open/close state
        ImGui::Checkbox("Another Window", &gs.showAnotherWindow);

        ImGui::SliderFloat(
            "float", &f, 0.0f,
            1.0f); // Edit 1 float using a slider from 0.0f to 1.0f
        ImGui::ColorEdit3(
            "clear color",
            (float*)&gs
                .bgColor); // Edit 3 floats representing a color

        if (ImGui::Button(
                "Button")) // Buttons return true when clicked (most widgets return true when edited/activated)
            counter++;
        ImGui::SameLine();
        ImGui::Text("counter = %d", counter);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                    1000.0f / gs.io().Framerate, gs.io().Framerate);
        ImGui::End();
    }

    if (gs.showAnotherWindow)
    {
        ImGui::Begin("Another Window");
        ImGui::Text("This is another window!");
        ImGui::End();
    }
}

void RefreshableSnapshotMenu::modifyPropertiesSubMenu() {
}

// TODO: Disable selection of a datatype if the size is too big for the snapshot.
// BUG TODO: Bounds checking. I am definitely not doing bounds checking rn.
// cannot be fucking assed.
void RefreshableSnapshotMenu::runMenu()
{
    using namespace std::chrono;
    // Used for keeping track of the current index in the snapshot for
    // gui creation.
    uintptr_t                  curOffset = 0;

    if (!ImGui::Begin(this->rs.mrp.regionName.c_str()))
    {
        ImGui::End();
    }
    if (ImGui::TreeNode("Region Properties"))
    {
        ImGui::Text("%s", this->rs.mrp.toStr().c_str());
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
                this->rs.refresh();
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
                this->rs.refresh();
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
        (this->rs.snap().size() > 8) ? 16 : this->rs.snap().size();
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
        flags, this->selection.Size, this->rs.snap().size());
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

    curOffset       = 0;
    size_t textCursorPos = 1;
    ImGui::TableNextColumn();
    char labelBuffer[32] = {0};
    while (curOffset < this->rs.snap().size())
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
            memcpy(&val, this->rs.snap().data() + curOffset,
                   curSize);
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
                memset(this->types.data() + initialOff, HEX, resetSize);
            }
            else
            {
                Log(Debug, "type hex -> unknown");
                memset(this->types.data() + initialOff, this->curType,
                       selSize);
            }
        }

        // // Print ascii conversion.
        if (curOffset % numCols == 0 || curOffset == this->rs.snap().size())
        {
            ImGui::TableNextColumn();
            char strVer[17] = {0};
            // Bounds check for bottom row lacking enough data.
            size_t boundsOffset = curOffset % numCols;
            if (boundsOffset == 0)
                boundsOffset = numCols;
            memcpy(strVer,
                   this->rs.snap().data() + curOffset - boundsOffset,
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
