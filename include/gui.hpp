#ifndef gui_hpp_INCLUDED
#define gui_hpp_INCLUDED

#include <chrono>
#include "core.hpp"
#include "imgui.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <optional>
#include <string>
#include <map>
#include "maps.hpp"
#include "snapshots.hpp"
#include "mem_anal.hpp"

struct MemoryRegionPropertiesPickerMenu {
};

struct AnalysisMenu {
    bool enabled;
    const pid_t pid;

    Program analysisState{};

    struct Comparator {
        bool operator()(const pid_t &pid1, const pid_t &pid2) const {
            return pid1 < pid2;
        }
    };

    void draw();
};

struct GuiState
{
    SDL_Window*                                    window;
    SDL_WindowFlags                                windowFlags;
    std::string                                    glslVersion;
    SDL_GLContext                                  glContext;

    float                                          mainScale;

    bool validState;
    bool showMemAnalWindow;
    bool                                           showDemoWindow;
    bool                                           showAnotherWindow;
    ImVec4                                         bgColor;

    std::map<pid_t, AnalysisMenu, AnalysisMenu::Comparator> analysisMenuList;

    ImGuiIO io;

    GuiState();

    void startFrame();

    void endFrame();

    // Draw the main window.
    void draw();
    
};

// TODO: Convert this table into something neater?
// The table is needed for the combo on the refreshable snasphot menu.
static const char* dataTypeTable[] = {
    "hex",      "char",    "uint8_t",  "int8_t",
    "uint16_t", "int16_t", "uint32_t", "int32_t",
    "uint64_t", "int64_t", "float",    "double",
};
static const char* dataConvTable[] = {"%02hhx", "%c",  "%hhu", "%hhd",
                                      "%hu",    "%hd", "%u",   "%d",
                                      "%lu",    "%ld", "%f",   "%lf"};

static const size_t dataSizeTable[] = {
    1, 1, 1, 1, 2, 2, 4, 4, 8, 8, 4, 8,
};

enum e_dataTypes : uint8_t
{
    HEX,
    CHAR,
    UINT8_T,
    INT8_T,
    UINT16_T,
    INT16_T,
    UINT32_T,
    INT32_T,
    UINT64_T,
    INT64_T,
    FLOAT,
    DOUBLE,
};

// Should this be owning? I don't think so.
struct RefreshableSnapshotMenu
{
    bool enabled = true;
private:
    // WARNING: Do NOT use the rs.refresh()!!! Use refreshSnapshot() instead because it updates the types
    // vector if necessary.
    RefreshableSnapshot      &rs;

    std::vector<e_dataTypes> types{};

    e_dataTypes              curType = HEX;

    ImGuiSelectionBasicStorage selection{};
    bool                       changeRequest = false;

    bool                       autoRefresh     = false;
    uint32_t                   refreshRateMS   = 1000;
    std::chrono::nanoseconds   lastRefreshTime = {};
    void modifyPropertiesSubMenu();

    void refreshSnapshot()
    {
        if (rs.mrp.relativeRegionSize != rs.snap().regionProperties.relativeRegionSize) {
            types.clear();
            types.resize(rs.mrp.relativeRegionSize, HEX);
        }
        rs.refresh();
    }

public:
    RefreshableSnapshotMenu(RefreshableSnapshot &rs)
    : rs(rs)
    {
        rs.refresh();
        types.resize(rs.snap().size(), HEX);
    }

    void draw();
};


#endif // gui_hpp_INCLUDED
