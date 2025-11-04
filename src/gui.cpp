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

static const std::array<size_t, 4> groupings = {8, 4, 2, 1}; // bytes
static const auto tableDefFlags = ImGuiTableFlags_SizingStretchSame |
    ImGuiTableFlags_NoClip | ImGuiTableFlags_NoBordersInBody;

void guiSnapshotTableFast(RefreshableSnapshotMenuState &rsms, const uint64_t data) {
}

// Always guarantee it to be 64 bit, and have the initial be static_casted.
// All values larger than the groupInd's will be truncated to fit.
// TODO: Nested tables are slow as shit for some reason. 
void guiSnapshotTableRecursive(RefreshableSnapshotMenuState& rsms,
                      const uint64_t data, const size_t groupInd)
{
    if (rsms.curOffset >= rsms.rs.snap().size())
        return;
    // Starts at 64, then 32, then 16, then 8
    const size_t sizeBytes = groupings[groupInd];
    size_t       numLoops  = 2;
    bool         baseCase  = false;

    if (dataSizeTable[rsms.types[rsms.curOffset]] == sizeBytes ||
        groupInd == groupings.size() - 1)
    {
        numLoops = 1;
        baseCase = true;
    }
    if (dataSizeTable[rsms.curType] == sizeBytes ||
        (rsms.types[rsms.curOffset] != HEX &&
         dataSizeTable[rsms.types[rsms.curOffset]] == sizeBytes))
    {
        char str[16] = {0};
        sprintf(str, "##ID: %lu", rsms.curOffset);
        bool selected =
            rsms.selection.Contains(rsms.curOffset);

        ImGui::SetNextItemAllowOverlap();
        ImGui::SetNextItemSelectionUserData(rsms.curOffset);
        ImGui::Selectable(str, selected);
        if (rsms.changeRequest && selected)
        {
            Log(Debug, "Selection ID: " << rsms.curOffset);
            if (rsms.types[rsms.curOffset] != HEX)
            {
                memset(rsms.types.data() + rsms.curOffset, HEX,
                       sizeBytes);
            }
            else
            {
                memset(rsms.types.data() + rsms.curOffset,
                       rsms.curType, sizeBytes);
            }
        }
        ImGui::SameLine();
    }

    if (ImGui::BeginTable(dataTypeTable[groupInd], numLoops,
                          tableDefFlags))
    {
        for (size_t i = 0; i < numLoops; i++)
        {
            ImGui::TableNextColumn();

            if (baseCase)
            {
                ImGui::Text(dataConvTable[rsms.types[rsms.curOffset]],
                            data);
                rsms.curOffset += sizeBytes;
            }
            else
            {
                // Get the size of the next version and truncate for next values.
                size_t         nextSize = groupings[groupInd + 1];
                const uint64_t defMask =
                    (~0ULL) >> ((8 - groupings[groupInd + 1]) * 8);
                uint64_t mask = defMask << (i * nextSize * 8);
                uint64_t truncatedData =
                    (data & mask) >> (i * nextSize * 8);
                guiSnapshotTableRecursive(rsms, truncatedData, groupInd + 1);
            }
        }
        ImGui::EndTable();
    }
}

// TODO: Fix stupid shit for a case where snapshot size is < 8;
void refreshableSnapshotMenu(RefreshableSnapshotMenuState& rsms)
{
    using namespace std::chrono;
    if (!ImGui::Begin(rsms.rs.mrp.regionName.c_str()))
    {
        ImGui::End();
        return;
    }
    ImGui::Text("%s\n", rsms.rs.mrp.toStr().c_str());

    ImGui::Checkbox("Toggle auto refreshing", &rsms.autoRefresh);
    if (!rsms.autoRefresh)
    {
        if (ImGui::Button("Refresh"))
        {
            rsms.rs.refresh();
        }
    }
    else
    {
        ImGui::DragScalar("Refresh Rate (ms)", ImGuiDataType_U32,
                          &rsms.refreshRateMS, 2.5f, 0, NULL,
                          "%u ms");
        if (steady_clock::now().time_since_epoch() -
                rsms.lastRefreshTime >
            milliseconds(rsms.refreshRateMS))
        {
            rsms.rs.refresh();
            rsms.lastRefreshTime =
                steady_clock::now().time_since_epoch();
        }
    }

    if (ImGui::BeginCombo("datatype chooser",
                          dataTypeTable[rsms.curType], 0))
    {
        for (size_t n = 0; n < IM_ARRAYSIZE(dataTypeTable); n++)
        {
            const bool isSelected = (rsms.curType == n);
            if (ImGui::Selectable(dataTypeTable[n], isSelected))
                rsms.curType = static_cast<e_dataTypes>(n);

            if (isSelected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    size_t numCols =
        (rsms.rs.snap().size() / groupings[0] > 2) ? 2 : 1;

    rsms.curOffset = 0;

    // +1 for the extra ascii view of the data.
    // Nested tables: Starts of with 8 byte sizes, 4 bytes, and 1 byte sized tables.
    // These tables are selectable (to some size, depending on the size of the chosen
    //                              datatype)
    if (ImGui::BeginChild(
            "##SnapView",
            ImVec2(-FLT_MIN, ImGui::GetFontSize() * 20)))
    {
        // Align the tables nicer with no padding.
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 1));
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0, 0));

        ImGuiMultiSelectFlags flags =
            ImGuiMultiSelectFlags_ClearOnEscape |
            ImGuiMultiSelectFlags_BoxSelect1d;
        ImGuiMultiSelectIO* ms_io =
            ImGui::BeginMultiSelect(flags, rsms.selection.Size,
                                    rsms.rs.mrp.relativeRegionSize);
        rsms.selection.ApplyRequests(ms_io);

        rsms.changeRequest = ImGui::Shortcut(ImGuiKey_Space) &&
            (rsms.selection.Size > 0);


        ImGuiListClipper clipper;
        size_t           numRows =
            rsms.rs.snap().size() / (groupings[0] * numCols) + 1;
        // clipper.Begin(numRows);

        if (ImGui::BeginTable("Snapshot Hex contents", numCols + 1,
                              ImGuiTableFlags_SizingStretchSame))
        {
            for (size_t i = 0;
                 i < rsms.rs.snap().size() / groupings[0]; i++)
            {
                // WARNING: This will not work with regions of less than 8 bytes!!!
                // FIXME: ^^^^^
                const uint64_t* value64 =
                    reinterpret_cast<const uint64_t*>(
                        rsms.rs.snap().data() + i * groupings[0]);
                ImGui::TableNextColumn();
                guiSnapshotTableRecursive(rsms, *value64, 0);

                // last col for a row, add an ascii thing to the end.
                if ((i + 1) % numCols == 0)
                {
                    char strVer[17] = {0};
                    int  offset     = ((int)i - 1) > 0 ? i - 1 : 0;
                    memcpy(strVer,
                           rsms.rs.snap().data() +
                               offset * groupings[0],
                           groupings[0] * numCols);
                    for (size_t j = 0; j < groupings[0] * numCols;
                         j++)
                    {
                        if (strVer[j] - 31 > 0)
                            continue;
                        strVer[j] = '.';
                    }

                    ImGui::TableNextColumn();
                    ImGui::Text("%s", strVer);
                    ImGui::TableNextRow();
                }
            }
        }
        ImGui::EndTable();
        ms_io = ImGui::EndMultiSelect();
        rsms.selection.ApplyRequests(ms_io);
    }
    ImGui::PopStyleVar();
    ImGui::PopStyleVar();

    ImGui::EndChild();
    ImGui::End();
}
