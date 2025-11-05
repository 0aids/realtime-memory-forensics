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
#include <set>
#include "snapshots.hpp"

struct GuiState
{
    SDL_Window*                                    window;
    SDL_WindowFlags                                windowFlags;
    std::string                                    glslVersion;
    SDL_GLContext                                  glContext;

    float                                          mainScale;

    bool                                           showDemoWindow;
    bool                                           showAnotherWindow;
    ImVec4                                         bgColor;

    std::optional<std::reference_wrapper<ImGuiIO>> _io;

    ImGuiIO&                                       io()
    {
        return _io.value().get();
    }
};

bool initGui(GuiState& gs);

void endGuiFrame(GuiState& gs);

void initGuiFrame();

void demoWindows(GuiState& gs);

struct RefreshableSnapshot
{
    std::optional<MemorySnapshot> _snap = {};
    MemoryRegionProperties        mrp;

    void                          refresh()
    {
        _snap.reset();
        _snap.emplace(makeSnapshotCore({.mrp = mrp}), mrp,
                      std::chrono::steady_clock::now().time_since_epoch());
    }

    MemorySnapshot& snap()
    {
        return _snap.value();
    }
};

static const char* dataTypeTable[] = {
    "hex",      "char",    "uint8_t",  "int8_t",
    "uint16_t", "int16_t", "uint32_t", "int32_t",
    "uint64_t", "int64_t", "float",    "double",
};
static const char* dataConvTable[] = {
    "%02hhx", "%c", "%hhu", "%hhd", 
    "%hu", "%hd", "%u", "%d", 
    "%lu", "%ld", "%f", "%lf"
};

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

struct RefreshableSnapshotMenuState
{
    RefreshableSnapshot rs;

    std::vector<e_dataTypes> types{};

    e_dataTypes curType = HEX;

    // Used for keeping track of the current index in the snapshot for
    // gui creation.
    uintptr_t curOffset = 0;

    ImGuiSelectionBasicStorage selection;
    size_t numRows;
    bool changeRequest = false;

    bool autoRefresh = false;
    uint32_t refreshRateMS = 1000;
    std::chrono::nanoseconds lastRefreshTime = {};
    // TODO: Get rid of this stupid init.
    void init() {
        types.resize(rs.snap().size(), HEX);
    }
};

void refreshableSnapshotMenu(RefreshableSnapshotMenuState &rsms);
void refreshableSnapshotMenuFast(RefreshableSnapshotMenuState &rsms);

#endif // gui_hpp_INCLUDED
