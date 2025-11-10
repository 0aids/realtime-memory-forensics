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

// NOTE: Do not use the mrp when looping through the snapshot.
// The snap holds the most recently used value.
namespace rmf::data
{
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
            rmf::backends::core::makeSnapshotCore({.mrp = mrp}));
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

