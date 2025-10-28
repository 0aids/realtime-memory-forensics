#ifndef core_hpp_defined
#define core_hpp_defined
#include <vector>
#include <unistd.h>
#include <cstdint>
#include <optional>
#include "maps.hpp"
#include "snapshots.hpp"

struct CoreInputs {
    template <typename T>
    // Optional reference
    using OptRef = std::optional<std::reference_wrapper<T>>;

    // This is all required informatino that could be necessary.
    std::optional<const MemoryRegionProperties> mrp = {};
    std::optional<std::span<const char>> snap1 = {};
    std::optional<std::span<const char>> snap2 = {};
};

std::vector<char> makeSnapshotCore(const CoreInputs& core);

// We need to know the parent region as well as its local offset.
// The copy of MemoryRegionProperties with the initial offset must be given.
std::vector<MemoryRegionProperties>
findChangedRegionsCore(const CoreInputs& core,
                       const uintptr_t &cmpSize);

#endif
