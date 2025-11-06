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
class ProgramAnalysisState {
public:
    const pid_t m_pid;
    RegionPropertiesList m_originalProperties;
    std::vector<RegionPropertiesList> m_rplHistory;

    std::vector<std::shared_ptr<RefreshableSnapshot>> m_highInterestSnapshots;

    std::vector<MemorySnapshot> m_lastSnaps1;
    std::vector<MemorySnapshot> m_lastSnaps2;

    ProgramAnalysisState(const pid_t pid) : m_pid(pid) {
    }
    // Need to provide a whole bunch of methods for modification our chosen snapshots?
    // Not really. The user has access to it as its public. not the best API but who cares.
    // I'm going to be the only user anyways.
    // The menu will modify the state directly.

    // Chunkifies the original properties, gets only readable snapshots and appends it to the history.
    void defaultInit() {
        m_rplHistory.emplace_back(
            breakIntoRegionChunks(
                m_originalProperties.filterRegionsByHasPerms("r"), 0) 
        );
    }

    std::span<MemorySnapshot> makeSnapshots(QueuedThreadPool tp)
    {
        std::vector<CoreInputs> coreInputs = consolidateIntoCoreInput({.mrpVec = m_rplHistory.back()});
                                                                       

        auto tasks = createMultipleTasks(makeSnapshotCore, coreInputs);
        tp.submitMultipleTasks(tasks);
        tp.awaitAllTasks();
        std::swap(m_lastSnaps1, m_lastSnaps2);
        m_lastSnaps1 = convertTasksIntoSnapshots(tasks);

        return std::span(m_lastSnaps1.data(), m_lastSnaps1.size());
    }

    template <typename CoreFuncType, typename... ExtraInputs>
    std::span<MemoryRegionProperties> findProperties(QueuedThreadPool tp, CoreFuncType coreFunc, ExtraInputs... extraInputs) {
        // Uses m_lastSnaps1;
        // Who cares about segmentation

        std::vector<CoreInputs> coreInputs = consolidateIntoCoreInput({.mrpVec = m_rplHistory.back(),
                                                                       .snap1Vec = makeSnapshotSpans(m_lastSnaps1),
                                                                       .snap2Vec = makeSnapshotSpans(m_lastSnaps2)});

        auto tasks = createMultipleTasks(coreFunc, coreInputs, extraInputs...);

        tp.submitMultipleTasks(tasks);
        tp.awaitAllTasks();
        m_rplHistory.push_back(
            consolidateNestedTaskResults(tasks)
        );

        return std::span(m_rplHistory.back().data(), m_rplHistory.back().size());
    }

    // Clear all original properties, current snapshots and high interest snapshots.
    void clearAll() {
        m_rplHistory.clear();
        m_lastSnaps1.clear();
        m_lastSnaps2.clear();
        // For menus created using this, automatically close the menu when the reference becomes invalid?
        // How do we know that the reference has become invalid? Make the refreshable menus hold a weak ptr to it.
        m_highInterestSnapshots.clear();
    }

    // Updates our original properties.
    void updateOriginalProperties()
    {
        m_originalProperties = readMapsFromPid(m_pid);
    }
};

#endif // mem_anal_h_INCLUDED
