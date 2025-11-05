#ifndef mem_anal_h_INCLUDED
#define mem_anal_h_INCLUDED

// The main api file
// This has all the good shit plus simplified api calls.

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
    }
};

// Holds the entire context.
// We will continuously append new RegionPropertiesList
// to a main vector containing our history.
// The original one will be held separately, and can be
// overwritten by refreshing the main maps.
class Program {
public:
    pid_t m_pid;
    RegionPropertiesList m_originalProperties;
    std::vector<RegionPropertiesList> m_rplHistory;

    std::vector<RefreshableSnapshot> m_highInterestSnapshots;

    std::vector<MemorySnapshot> m_lastSnaps1;
    std::vector<MemorySnapshot> m_lastSnaps2;

    std::vector<CoreInputs> m_nextInputs;

    void populateInputs();

    // Clear all original properties, current snapshots and high interest snapshots.
    void clearAll();

    // Updates our original properties.
    void updateOriginalProperties();
};

#endif // mem_anal_h_INCLUDED
