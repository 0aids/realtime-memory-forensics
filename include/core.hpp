#ifndef core_hpp_defined
#define core_hpp_defined
#include <concepts>
#include "logs.hpp"
#include <vector>
#include <unistd.h>
#include <cstdint>
#include <optional>
#include "maps.hpp"
#include "snapshots.hpp"

struct CoreInputs {
    // This is all required informatino that could be necessary.
    std::optional<const MemoryRegionProperties> mrp = {};
    std::optional<MemorySnapshotSpan> snap1 = {};
    std::optional<MemorySnapshotSpan> snap2 = {};
};

std::vector<char> makeSnapshotCore(const CoreInputs& core);

// We need to know the parent region as well as its local offset.
// The copy of MemoryRegionProperties with the initial offset must be given.
std::vector<MemoryRegionProperties>
findChangedRegionsCore(const CoreInputs& core,
                       const uintptr_t &cmpSize);

std::vector<MemoryRegionProperties>
findStringCore(
    const CoreInputs& core,
    const std::string_view &str
);

std::vector<MemoryRegionProperties>
findUnchangedRegionsCore(const CoreInputs& core, const uintptr_t& cmpSize);

template <typename T>
concept Numeric = std::integral<T> || std::floating_point<T>;

template <Numeric NumType>
std::vector<MemoryRegionProperties>
findNumericWithinRange(const CoreInputs& core,
                       const NumType &max, const NumType &min)
{
    if (!core.snap1)
    {
        Log(Error, "Missing a snapshot!");
        return {};
    }
    const auto&                   snap1    = core.snap1.value();
    const MemoryRegionProperties& mrp      = snap1.regionProperties;
    size_t                        numIters = snap1.size() / sizeof(NumType);
    // uintptr_t                     current  = 0;
    // Ensure proper alignment.
    uintptr_t current = mrp.getActualRegionStart() - (mrp.getActualRegionStart() / sizeof(NumType)) * sizeof(NumType);
    Log(Debug, "Number of iterations required: " << numIters);

    std::vector<MemoryRegionProperties> data;
    for (size_t i = 0; i < numIters; ++i)
    {
        uintptr_t curSize =
            (i == numIters - 1) ? snap1.size() - current : sizeof(NumType);

        NumType d1;
        memcpy(&d1, snap1.data() + current, curSize);

        if (d1 <= max && d1 >= min)
        {
            Log(Message, "Found negligible change!");
            MemoryRegionProperties nmrp = mrp;
            nmrp.relativeRegionSize     = curSize;
            nmrp.relativeRegionStart += current;
            data.push_back(std::move(nmrp));
        }
        current += curSize;
    }
    Log(Debug, "Found changes: " << data.size());
    return data;
}

template <Numeric NumType>
std::vector<MemoryRegionProperties>
findChangedNumericCore(const CoreInputs& core,
                       const NumType &minDiff)
{
    if (!core.snap1 || !core.snap2 || !core.mrp)
    {
        Log(Error, "Missing a snapshot!");
        return {};
    }
    const auto&                   snap1    = core.snap1.value();
    const auto&                   snap2    = core.snap2.value();
    const MemoryRegionProperties& mrp      = core.mrp.value();
    size_t                        numIters = snap1.size() / sizeof(NumType);
    // uintptr_t                     current  = 0;
    uintptr_t current = mrp.getActualRegionStart() - (mrp.getActualRegionStart() / sizeof(NumType)) * sizeof(NumType);
    Log(Debug, "Number of iterations required: " << numIters);
    Log(Debug, "Starting span offset: " << std::hex << std::showbase << current);

    std::vector<MemoryRegionProperties> data;
    for (size_t i = 0; i < numIters; ++i)
    {
        uintptr_t curSize =
            (i == numIters - 1) ? snap1.size() - current : sizeof(NumType);

        NumType d1;
        NumType d2;
        // I hope no seg faults
        memcpy(&d1, snap1.data() + current, curSize);
        memcpy(&d2, snap2.data() + current, curSize);

        if (fabs((double) d1 - (double) d2) >= minDiff)
        {
            Log(Message, "Found a change!");
            MemoryRegionProperties nmrp = mrp;
            nmrp.relativeRegionSize     = curSize;
            nmrp.relativeRegionStart += current;
            data.push_back(std::move(nmrp));
        }
        current += curSize;
    }
    Log(Debug, "Found changes: " << data.size());
    return data;
}

template <Numeric NumType>
std::vector<MemoryRegionProperties>
findUnchangedNumericCore(const CoreInputs& core,
                       const NumType &maxDiff)
{
    if (!core.snap1 || !core.snap2 || !core.mrp)
    {
        Log(Error, "Missing a snapshot!");
        return {};
    }
    const auto&                   snap1    = core.snap1.value();
    const auto&                   snap2    = core.snap2.value();
    const MemoryRegionProperties& mrp      = core.mrp.value();
    size_t                        numIters = snap1.size() / sizeof(NumType);
    // uintptr_t                     current  = 0;
    uintptr_t current = mrp.getActualRegionStart() - (mrp.getActualRegionStart() / sizeof(NumType)) * sizeof(NumType);
    Log(Debug, "Number of iterations required: " << numIters);

    std::vector<MemoryRegionProperties> data;
    for (size_t i = 0; i < numIters; ++i)
    {
        uintptr_t curSize =
            (i == numIters - 1) ? snap1.size() - current : sizeof(NumType);

        NumType d1;
        NumType d2;
        memcpy(&d1, snap1.data() + current, curSize);
        memcpy(&d2, snap2.data() + current, curSize);

        if (fabs((double) d1 - (double) d2) <= maxDiff)
        {
            Log(Message, "Found negligible change!");
            MemoryRegionProperties nmrp = mrp;
            nmrp.relativeRegionSize     = curSize;
            nmrp.relativeRegionStart += current;
            data.push_back(std::move(nmrp));
        }
        current += curSize;
    }
    Log(Debug, "Found changes: " << data.size());
    return data;
}
#endif
