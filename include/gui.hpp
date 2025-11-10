#ifndef gui_hpp_INCLUDED
#define gui_hpp_INCLUDED

#include <chrono>
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <optional>
#include <string>
#include <map>
#include "imgui.h"
#include "backends/core.hpp"
#include "data/maps.hpp"
#include "data/snapshots.hpp"
#include "data/refreshable_snapshots.hpp"
#include "tests.hpp"

namespace rmf::gui {
    using namespace rmf::data;
    using namespace rmf::tests;
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
    bool enabled = true;
private:
    // WARNING: Do NOT use the rs.refresh()!!! Use refreshSnapshot() instead because it updates the types
    // vector if necessary.
    std::weak_ptr<RefreshableSnapshot>      m_rs_wptr;

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
        auto data = m_rs_wptr.lock();
        if (!data)
        {
            enabled = false;
            return;
        }
        if (data->mrp.relativeRegionSize != data->snap().regionProperties.relativeRegionSize) {
            types.clear();
            types.resize(data->mrp.relativeRegionSize, HEX);
        }
        data->refresh();
    }

public:
    RefreshableSnapshotMenu(std::shared_ptr<RefreshableSnapshot> &rs_sptr)
    {
        m_rs_wptr = rs_sptr;
        types.resize(rs_sptr->snap().size(), HEX);
    }

    void draw();
};

struct DemoTestState{
    // Demo and testing stuff.
    // AnalysisMenu demoAnalysisMenu;
    std::shared_ptr<RefreshableSnapshot> rs_sptr;
    std::shared_ptr<RefreshableSnapshot> crs_sptr;
    RefreshableSnapshotMenu rsms;
    RefreshableSnapshotMenu crsms;

    DemoTestState(
        pid_t pid
    )
    : // demoAnalysisMenu(pid),
    rs_sptr(getSampleRefreshableSnapshots(pid).first),
    crs_sptr(getSampleRefreshableSnapshots(pid).second),
    rsms(RefreshableSnapshotMenu(rs_sptr)),
    crsms(RefreshableSnapshotMenu(crs_sptr))
    {
        rmf_Log(rmf_Debug, "Initialized the demo test state!");
    }

    void draw() {
        // demoAnalysisMenu.draw();
        rsms.draw();
        crsms.draw();
    }
};

struct GuiState
{
    SDL_Window*                                    window;
    SDL_WindowFlags                                windowFlags;
    std::string                                    glslVersion;
    SDL_GLContext                                  glContext;

    float                                          mainScale;
    ImVec4                                         bgColor;

    bool validState;
    bool showMemAnalWindow;
    // std::map<pid_t, AnalysisMenu, AnalysisMenu::Comparator> analysisMenuList;

    bool                                           showDemoWindow;
    bool                                           showAnotherWindow;
    std::optional<DemoTestState> opt_demoTestState;

    ImGuiIO io;

    GuiState(std::optional<pid_t> opt_pid={});

    void startFrame();

    void endFrame();

    // Draw the main window.
    void draw();
    
};

}


#endif // gui_hpp_INCLUDED

