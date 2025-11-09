#ifndef gui_hpp_INCLUDED
#define gui_hpp_INCLUDED

#include <chrono>
#include "core.hpp"
#include "imgui.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <optional>
#include <string>
#include <map>
#include "maps.hpp"
#include "snapshots.hpp"
#include "mem_anal.hpp"
#include "tests.hpp"
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

struct MemoryRegionPropertiesPickerMenu {
};


struct GuiInstruction {
    int instIndex = 0;
    int ID = 0;
    // All possible inputs. Dunno of a cleaner way.
    // Should hopefully be enough
    char inputText[1024] = {0};
    int registerInd1 = 0;
    int registerInd2 = 0;
    int min = 0;
    int max = 0;
    double mind = 0;
    double maxd = 0;
    int commandListInd = 0;
    int rplTimelineInd = 0;
    int integer = 0;
};

ProgramAnalysisState::Instruction convertGuiInstruction(const GuiInstruction &ginst);

struct AnalysisMenu {
    bool enabled;
    const pid_t pid;
    int IDCounter = 0;

    // Actually owns and modifies this, unlike the views.
    std::shared_ptr<ProgramAnalysisState> analysisState_sptr{};
    std::vector<GuiInstruction> m_curInstructionList;

    struct Comparator {
        bool operator()(const pid_t &pid1, const pid_t &pid2) const {
            return pid1 < pid2;
        }
    };

    AnalysisMenu(pid_t pid): pid(pid) {
        enabled = true;
        analysisState_sptr = std::make_shared<ProgramAnalysisState>(pid);
    }

    void draw();

    private:
        void drawOGMapsTab();
        void drawFilterRegionsTab();
        void drawSnapshotsTab();
        void drawCoreFuncTab();

        // Returns -1 if the selected instruction wants to move upwards
        // And +1 for downwards (index based)
        // 0 for none.
        int drawInstructionMenu(GuiInstruction &inst);


        // The entire tab, calls drawInstructionfmenu for each.
        void drawInstructionTab();

        void drawArgumentPicker(GuiInstruction &inst);
        // Sends the instructionList to the shared state.
        void sendCommand();
};

struct DemoTestState{
    // Demo and testing stuff.
    AnalysisMenu demoAnalysisMenu;
    std::shared_ptr<RefreshableSnapshot> rs_sptr;
    std::shared_ptr<RefreshableSnapshot> crs_sptr;
    RefreshableSnapshotMenu rsms;
    RefreshableSnapshotMenu crsms;

    DemoTestState(
        pid_t pid
    )
    : demoAnalysisMenu(pid),
    rs_sptr(getSampleRefreshableSnapshots(pid).first),
    crs_sptr(getSampleRefreshableSnapshots(pid).second),
    rsms(RefreshableSnapshotMenu(rs_sptr)),
    crsms(RefreshableSnapshotMenu(crs_sptr))
    {
        Log(Debug, "Initialized the demo test state!");
    }

    void draw() {
        demoAnalysisMenu.draw();
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
    std::map<pid_t, AnalysisMenu, AnalysisMenu::Comparator> analysisMenuList;

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



#endif // gui_hpp_INCLUDED
