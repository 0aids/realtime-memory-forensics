#ifndef gui_hpp_INCLUDED
#define gui_hpp_INCLUDED

#include "imgui_internal.h"
#endif // gui_hpp_INCLUDED
#include <imgui.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_sdl3.h"
#include <imnodes.h>
#include <string>
#include <optional>
#include <vector>
#include <stack>
#include <algorithm>
#include "gui_examples.hpp"

namespace rmf::gui
{

    struct GuiState
    {
        SDL_Window*     window;
        SDL_WindowFlags windowFlags;
        std::string     glslVersion;
        SDL_GLContext   glContext;

        float           mainScale;
        ImVec4          bgColor;

        bool            validState;
        bool            showMemAnalWindow;
        // std::map<pid_t, AnalysisMenu, AnalysisMenu::Comparator> analysisMenuList;

        bool    showDemoWindow;
        bool    showAnotherWindow;

        ImGuiIO io;

        examples::ColorNodeEditor exampleNodeEditor;
        bool showExampleNodeEditor = true;

        GuiState(std::optional<pid_t> opt_pid = {});
        ~GuiState();

        void startFrame();

        void endFrame();

        // Draw the main window.
        void draw();
    };

}
