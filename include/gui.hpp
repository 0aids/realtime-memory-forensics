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

namespace rmf::gui
{

    enum class ScanOperation
    {
        ExactValue,
        Changed,
        Unchanged,
        String,
        PointerToRegion
    };

    enum class ScanValueType
    {
        i8,
        i16,
        i32,
        i64,
        u8,
        u16,
        u32,
        u64,
        f32,
        f64,
        string
    };

    struct ScanResult
    {
        uintptr_t                     address;
        std::string                   valueStr;
        types::MemoryRegionProperties mrp;
    };

    struct GuiState
    {
        SDL_Window*               window;
        SDL_WindowFlags           windowFlags;
        std::string               glslVersion;
        SDL_GLContext             glContext;

        float                     mainScale;
        ImVec4                    bgColor;

        bool                      validState;
        bool                      showMemAnalWindow;

        bool                      showDemoWindow;
        bool                      showAnotherWindow;

        bool                      showProcessWindow       = true;
        bool                      showMemoryRegionsWindow = true;

        ImGuiIO                   io;

        examples::ColorNodeEditor exampleNodeEditor;
        bool                      showExampleNodeEditor = false;

        graph::MemoryGraphViewer  mgViewerTest;
        bool                      showMemoryGraphViewerTest = true;

        // Analysis state
        std::optional<pid_t>             targetPid;
        std::string                      targetProcessName;
        size_t                           analyzerThreadCount;
        std::unique_ptr<rmf::Analyzer>   analyzer;
        types::MemoryRegionPropertiesVec currentRegions;
        std::string                      regionFilter;
        int                              selectedRegionIndex;

        // Process browser
        std::vector<std::pair<pid_t, std::string>> processList;
        void refreshProcessList();
        void attachToProcess(pid_t pid);
        void detachFromProcess();

        // Scan state
        bool          showScanWindow   = true;
        ScanOperation scanOperation    = ScanOperation::ExactValue;
        ScanValueType scanValueType    = ScanValueType::i32;
        char          scanValueBuf[64] = "";
        bool          scanHexInput     = false;
        bool          isScanning       = false;
        std::vector<ScanResult> scanResults;
        int                     selectedResultIndex = -1;
        std::chrono::steady_clock::time_point scanStartTime;
        std::chrono::milliseconds             scanDuration{0};

        void                                  runScan();
        void                                  clearScanResults();

        GuiState(std::optional<pid_t> opt_pid = {});
        ~GuiState();

        void startFrame();

        void endFrame();

        // Draw the main window.
        void draw();
    };

}
#endif // gui_hpp_INCLUDED
