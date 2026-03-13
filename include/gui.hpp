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
#include <vector>
#include "gui_examples.hpp"
#include "memory_graph_viewer.hpp"
#include "rmf.hpp"
#include "types.hpp"
#include "utils.hpp"
#include "gui_types.hpp"
#include "process_manager.hpp"
#include "scan_manager.hpp"
#include "graph_manager.hpp"

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

        bool            showDemoWindow;
        bool            showAnotherWindow;

        // Managers
        std::unique_ptr<ProcessManager> processManager;
        std::unique_ptr<ScanManager>    scanManager;
        std::unique_ptr<GraphManager>   graphManager;

        // UI State
        ImGuiIO        io;
        ImGuiID        dockspaceId;
        ImGuiViewport* viewport;

        // Windows visibility
        bool showProcessWindow       = true;
        bool showMemoryRegionsWindow = true;
        bool showScanWindow          = true;
        bool showNodeViewer          = true;

        GuiState(std::optional<pid_t> opt_pid = {});
        ~GuiState();

        void startFrame();

        void endFrame();

        // Draw the main window.
        void draw();
    };

}
#endif // gui_hpp_INCLUDED
