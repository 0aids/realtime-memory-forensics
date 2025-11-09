#ifndef mem_anal_h_INCLUDED
#define mem_anal_h_INCLUDED

// The main api file
// This has all the good shit plus simplified api calls.

#include "core_wrappers.hpp"
#include "maps.hpp"
#include "snapshots.hpp"
#include <sched.h>
#include <vector>
#include "core.hpp"


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
            makeSnapshotCore({.mrp = mrp}));
    }

    const MemorySnapshot& snap()
    {
        return _snap.value();
    }

    RefreshableSnapshot(const MemoryRegionProperties &mrp) : mrp(mrp)
    {
        refresh();
    }
};

enum e_InputConfigurations {
    MAKE_SNAPSHOTS,
    FIND_IN_SNAPSHOTS,
    CMP_SNAPSHOTS,
};

// Holds the entire context. Is basically a wrapper for ease of use.
// We will continuously append new RegionPropertiesList
// to a main vector containing our history.
// The original one will be held separately, and can be
// overwritten by refreshing the main maps.
// It runs on instructions, which are just a list of void() that modify the state,
class ProgramAnalysisState {
public:
    const pid_t m_pid;
    RegionPropertiesList m_originalProperties;
    std::vector<RegionPropertiesList> m_rplTimeline;

    std::vector<std::shared_ptr<RefreshableSnapshot>> m_highInterestSnapshots;

    size_t rplTimelineIndex = 0;

    // Running an instruction from the timeline that is not the end will
    // copy it to the end.
    size_t instListTimelineIndex = 0;

    std::vector<MemorySnapshot> m_lastSnaps1;
    std::vector<MemorySnapshot> m_lastSnaps2;

    // An instruction
    // The menu will have predefined selectable/reorderable submenus which only edit specific
    // parts of the text, which will then be compiled and run afterwards.
    // This means less code overall.
    struct Instruction {
        std::function<void(ProgramAnalysisState &)> compiledText;
        std::string text;

        void parseLine();
    };

    using InstructionList = std::vector<Instruction>;
    std::vector<InstructionList> m_instListTimeline;

    ProgramAnalysisState(const pid_t pid) : m_pid(pid) {
    }

    // Runs the most recent list.
    void runInstructions() {
        InstructionList &l = m_instListTimeline.back();
        for (auto &i : l) {
            i.parseLine();
            i.compiledText(*this);
        }
    }

    std::span<MemorySnapshot> makeSnapshots(QueuedThreadPool &tp)
    {
        std::vector<CoreInputs> coreInputs = consolidateIntoCoreInput({.mrpVec = m_rplTimeline.back()});
                                                                       

        auto tasks = createMultipleTasks(makeSnapshotCore, coreInputs);
        tp.submitMultipleTasks(tasks);
        tp.awaitAllTasks();
        std::swap(m_lastSnaps1, m_lastSnaps2);
        m_lastSnaps1 = convertTasksIntoSnapshots(tasks);

        return std::span(m_lastSnaps1.data(), m_lastSnaps1.size());
    }

    template <typename CoreFuncType, typename... ExtraInputs>
    std::span<MemoryRegionProperties> findProperties(QueuedThreadPool &tp, CoreFuncType coreFunc, ExtraInputs... extraInputs) {
        // Uses m_lastSnaps1;
        // Who cares about segmentation

        std::vector<CoreInputs> coreInputs = consolidateIntoCoreInput({.mrpVec = m_rplTimeline.back(),
                                                                       .snap1Vec = makeSnapshotSpans(m_lastSnaps1),
                                                                       .snap2Vec = makeSnapshotSpans(m_lastSnaps2)});

        auto tasks = createMultipleTasks(coreFunc, coreInputs, extraInputs...);

        tp.submitMultipleTasks(tasks);
        tp.awaitAllTasks();
        m_rplTimeline.push_back(
            consolidateNestedTaskResults(tasks)
        );

        return std::span(m_rplTimeline.back().data(), m_rplTimeline.back().size());
    }

    // Clear all original properties, current snapshots and high interest snapshots.
    void clearAll() {
        m_rplTimeline.clear();
        m_lastSnaps1.clear();
        m_lastSnaps2.clear();
        m_originalProperties.clear();
        // For menus created using this, automatically close the menu when the reference becomes invalid?
        // How do we know that the reference has become invalid? Make the refreshable menus hold a weak ptr to it.
        m_highInterestSnapshots.clear();
        m_instListTimeline.clear();
    }

    // Updates our original properties.
    void updateOriginalProperties()
    {
        m_originalProperties = readMapsFromPid(m_pid);
    }

    void copyOriginalPropertiesToTimeline() {
        Log(Debug, "Copying to list timeline!");
        if (m_originalProperties.size() == 0) {
            Log(Warning, "There are no properties to copy to timeline");
            return;
        }
        m_rplTimeline.push_back(m_originalProperties);
    }
};

#endif // mem_anal_h_INCLUDED
