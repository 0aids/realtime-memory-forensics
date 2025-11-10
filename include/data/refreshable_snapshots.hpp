#ifndef mem_anal_h_INCLUDED
#define mem_anal_h_INCLUDED

// The main api file
// This has all the good shit plus simplified api calls.

#include "backends/mt_backend.hpp"
#include "maps.hpp"
#include "snapshots.hpp"
#include <sched.h>
#include <vector>
#include "backends/core.hpp"

namespace rmf::data
{
    using namespace rmf::backends::core;
struct RefreshableSnapshot
{
  private:
    std::optional<MemorySnapshot> _snap = {};

  public:
    // NOTE: Do not use the mrp when looping through the snapshot.
    // The snap holds the most recently used value.
    // THIS COULD BE OUT OF DATE DUE TO BEING MUTABLE
    MemoryRegionProperties mrp;

    void                   refresh()
    {
        _snap.reset();
        _snap.emplace(
            makeSnapshotCore(CoreInputs(mrp)));
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
};


#endif // mem_anal_h_INCLUDED

