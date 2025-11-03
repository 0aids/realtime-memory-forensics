#ifndef mem_anal_h_INCLUDED
#define mem_anal_h_INCLUDED

// The main api file
// This has all the good shit plus simplified api calls.

#include "maps.hpp"
#include "snapshots.hpp"
#include <sched.h>
#include <vector>

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

    // Optional to allow destruction and reconstruction in place,
    // without having to remove const correctness of snapshots
    std::vector<std::optional<MemorySnapshot>> m_highInterestSnapshots;

    // A current list of the ones that we are doing.
    std::vector<MemorySnapshot> m_currentSnapshots;

    void clearAll();
    void updateOriginalProperties();
};

#endif // mem_anal_h_INCLUDED
