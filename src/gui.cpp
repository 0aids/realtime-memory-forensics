#include "imnodes.h"
#include "utils.hpp"
#include <memory>
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

        if (opt_pid.has_value())
        {
            // TODO: Do something with the PID
        }
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
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("Main Menu"))
            {
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Demo stuff"))
            {
                ImGui::MenuItem("Show Demo", NULL,
                                &this->showDemoWindow);
                // ImGui::MenuItem("Show Demo Node Editor", NULL,
                //                 &this->showExampleNodeEditor);
                ImGui::MenuItem("Show extra window", NULL,
                                &this->showAnotherWindow);
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
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

        // if (this->showExampleNodeEditor)
        // {
        //     this->exampleNodeEditor.show(
        //         &this->showExampleNodeEditor);
        // }

        if (this->showMemoryGraphViewerTest)
        {
            ImGui::Begin("MGViewer test",
                         &this->showMemoryGraphViewerTest);
            this->mgViewerTest.draw();
            ImGui::End();
        }
    }
}
