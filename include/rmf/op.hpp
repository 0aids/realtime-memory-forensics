#pragma once
#include "rmf/config.hpp"
#include "rmf/snapshots.hpp"
#include "rmf/memory_region.hpp"

#include <cassert>

namespace rmf
{

    std::vector<MemoryRegion> findChanged(MemoryRegionView in1,
                                          MemoryRegionView in2,
                                          uintptr_t        compareSize);

    std::vector<MemoryRegion> findUnchanged(MemoryRegionView in1,
                                            MemoryRegionView in2,
                                            uintptr_t        compareSize);

    template <Numeric N>
    std::vector<MemoryRegion> findNumChanged(MemoryRegionView in1,
                                             MemoryRegionView in2,
                                             N minChangeRequired);

    template <Numeric N>
    std::vector<MemoryRegion> findNumUnchanged(MemoryRegionView in1,
                                               MemoryRegionView in2,
                                               N maxChangeRequired);

    std::vector<MemoryRegion> findString(MemoryRegionView       in1,
                                         const std::string_view str);

    template <Numeric N>
    std::vector<MemoryRegion> findNumExact(MemoryRegionView in1, N num);

    template <Numeric N>
    std::vector<MemoryRegion> findNumInRange(MemoryRegionView in1, N min,
                                             N max);
} // namespace rmf

namespace rmf
{
    std::vector<MemoryRegion> findChanged(MemoryRegionView in1,
                                          MemoryRegionView in2,
                                          uintptr_t        compareSize)

    {
        assert(false && "TODO!");
    }

    std::vector<MemoryRegion> findUnchanged(MemoryRegionView in1,
                                            MemoryRegionView in2,
                                            uintptr_t        compareSize)

    {
        assert(false && "TODO!");
    }

    template <Numeric N>
    std::vector<MemoryRegion> findNumChanged(MemoryRegionView in1,
                                             MemoryRegionView in2,
                                             N                minChangeRequired)

    {
        assert(false && "TODO!");
    }

    template <Numeric N>
    std::vector<MemoryRegion> findNumUnchanged(MemoryRegionView in1,
                                               MemoryRegionView in2,
                                               N maxChangeRequired)
    {
        assert(false && "TODO!");
    }

    std::vector<MemoryRegion> findString(MemoryRegionView       in1,
                                         const std::string_view str)
    {
        assert(false && "TODO!");
    }

    template <Numeric N>
    std::vector<MemoryRegion> findNumExact(MemoryRegionView in1, N num)
    {
        assert(false && "TODO!");
    }

    template <Numeric N>
    std::vector<MemoryRegion> findNumInRange(MemoryRegionView in1, N min, N max)
    {
        assert(false && "TODO!");
    }
}
