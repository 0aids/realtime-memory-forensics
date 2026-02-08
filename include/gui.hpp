#ifndef gui_hpp_INCLUDED
#define gui_hpp_INCLUDED

#include "imgui_internal.h"
#include <imgui.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_sdl3.h"
#include <imnodes.h>
#include <string>
#include <optional>
#include "gui_examples.hpp"
#include "memory_graph_viewer.hpp"

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

        bool    showDemoWindow;
        bool    showAnotherWindow;

        ImGuiIO io;

        examples::ColorNodeEditor exampleNodeEditor;
        bool showExampleNodeEditor = false;

        graph::MemoryGraphViewer mgViewerTest;
        bool showMemoryGraphViewerTest = true;

        GuiState(std::optional<pid_t> opt_pid = {});
        ~GuiState();

        void startFrame();

        void endFrame();

        // Draw the main window.
        void draw();
    };

}
#endif // gui_hpp_INCLUDED
