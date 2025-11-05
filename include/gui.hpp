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
#include "maps.hpp"
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

// NOTE: Do not use the mrp when looping through the snapshot.
// The snap holds the most recently used value.
struct RefreshableSnapshot
{
  private:
    std::optional<MemorySnapshot> _snap = {};

  public:
    MemoryRegionProperties mrp;

    void                   refresh()
    {
        _snap.reset();
        _snap.emplace(
            makeSnapshotCore({.mrp = mrp}), mrp,
            std::chrono::steady_clock::now().time_since_epoch());
    }

    const MemorySnapshot& snap()
    {
        return _snap.value();
    }

    RefreshableSnapshot(const MemoryRegionProperties &mrp) : mrp(mrp)
    {
    }
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

struct RefreshableSnapshotMenu
{
private:
    RefreshableSnapshot      rs;

    std::vector<e_dataTypes> types{};

    e_dataTypes              curType = HEX;

    ImGuiSelectionBasicStorage selection{};
    bool                       changeRequest = false;

    bool                       autoRefresh     = false;
    uint32_t                   refreshRateMS   = 1000;
    std::chrono::nanoseconds   lastRefreshTime = {};
    void modifyPropertiesSubMenu();

public:
    RefreshableSnapshotMenu(const MemoryRegionProperties &mrp)
    : rs(mrp)
    {
        rs.refresh();
        types.resize(rs.snap().size(), HEX);
    }

    void runMenu();
};

#endif // gui_hpp_INCLUDED
