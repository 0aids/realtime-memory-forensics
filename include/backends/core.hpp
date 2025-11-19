#ifndef core_hpp_defined
#define core_hpp_defined
#include <concepts>
#include <ranges>
#include <vector>
#include <unistd.h>
#include <cstdint>
#include <optional>
#include "data/numerics.hpp"
#include "utils/logs.hpp"
#include "data/maps.hpp"
#include "data/snapshots.hpp"

namespace rmf::backends::core
{
    using namespace rmf::data;
    struct CoreInputs
    {
        // This is all required informatino that could be necessary.
        std::optional<const MemoryRegionProperties> mrp   = {};
        std::optional<const MemorySnapshotSpan>           snap1 = {};
        std::optional<const MemorySnapshotSpan>           snap2 = {};
        CoreInputs() = default;
        CoreInputs(const MemorySnapshotSpan &span1): snap1(span1) {}
        CoreInputs(const MemorySnapshotSpan &span1, const MemorySnapshotSpan &span2): snap1(span1), snap2(span2) {}
        CoreInputs(MemoryRegionProperties mrp): mrp(mrp) {}
    };

    MemorySnapshot makeSnapshotCore(const CoreInputs& core);

    // We need to know the parent region as well as its local offset.
    // The copy of MemoryRegionProperties with the initial offset must be given.
    std::vector<MemoryRegionProperties>
    findChangedRegionsCore(const CoreInputs& core,
                           const uintptr_t&  cmpSize);

    std::vector<MemoryRegionProperties>
    findStringCore(const CoreInputs&       core,
                   const std::string_view& str);

    std::vector<MemoryRegionProperties>
    findUnchangedRegionsCore(const CoreInputs& core,
                             const uintptr_t&  cmpSize);

    template <typename T>
    concept Numeric = std::integral<T> || std::floating_point<T>;

    // Use these for numerics, esp for runtime.
    std::vector<MemoryRegionProperties>
    findNumericWithinRange(const CoreInputs& core, 
                           const NumQuery& query);


    std::vector<MemoryRegionProperties>
    findChangedNumeric(const CoreInputs& core,
                           const NumQuery& query);

    std::vector<MemoryRegionProperties>
    findUnchangedNumeric(const CoreInputs& core,
                           const NumQuery& query);

    template <std::ranges::contiguous_range Range, Numeric NumType>
    inline NumType numCast(const Range &range) {
        NumType result;
        memcpy(&result, range.data(), sizeof(result));
        return result;
    }

    template <Numeric NumType>
    std::vector<MemoryRegionProperties>
    _findNumericWithinRange(const CoreInputs& core, const NumType& min,
                           const NumType& max)
    {
        if (!core.snap1)
        {
            rmf_Log(rmf_Error, "Missing a snapshot!");
            return {};
        }
        const auto&                   snap1 = core.snap1.value();
        const MemoryRegionProperties& mrp   = snap1.regionProperties;
        size_t numIters = snap1.size() / sizeof(NumType);
        // uintptr_t                     current  = 0;
        // Ensure proper alignment.
        uintptr_t current = mrp.getActualRegionStart() -
            (mrp.getActualRegionStart() / sizeof(NumType)) *
                sizeof(NumType);
        rmf_Log(rmf_Debug, "Number of iterations required: " << numIters);

        std::vector<MemoryRegionProperties> data;
        for (size_t i = 0; i < numIters; ++i)
        {
            uintptr_t curSize = (i == numIters - 1) ?
                snap1.size() - current :
                sizeof(NumType);

            NumType   d1;
            memcpy(&d1, snap1.data() + current, curSize);

            if (d1 <= max && d1 >= min)
            {
                rmf_Log(rmf_Message, "Found negligible change!");
                MemoryRegionProperties nmrp = mrp;
                nmrp.relativeRegionSize     = curSize;
                nmrp.relativeRegionStart += current;
                data.push_back(std::move(nmrp));
            }
            current += curSize;
        }
        rmf_Log(rmf_Debug, "Found changes: " << data.size());
        return data;
    }

    template <Numeric NumType>
    std::vector<MemoryRegionProperties>
    _findChangedNumericCore(const CoreInputs& core,
                           const NumType&    minDiff)
    {
        if (!core.snap1 || !core.snap2)
        {
            rmf_Log(rmf_Error, "Missing a snapshot!");
            return {};
        }
        const auto&                   snap1 = core.snap1.value();
        const auto&                   snap2 = core.snap2.value();
        const MemoryRegionProperties& mrp   = snap1.regionProperties;
        size_t numIters = snap1.size() / sizeof(NumType);
        // uintptr_t                     current  = 0;
        uintptr_t current = mrp.getActualRegionStart() -
            (mrp.getActualRegionStart() / sizeof(NumType)) *
                sizeof(NumType);
        rmf_Log(rmf_Debug, "Number of iterations required: " << numIters);
        rmf_Log(rmf_Debug,
            "Starting span offset: " << std::hex << std::showbase
                                     << current);

        std::vector<MemoryRegionProperties> data;
        for (size_t i = 0; i < numIters; ++i)
        {
            uintptr_t curSize = (i == numIters - 1) ?
                snap1.size() - current :
                sizeof(NumType);

            NumType   d1;
            NumType   d2;
            // I hope no seg faults
            memcpy(&d1, snap1.data() + current, curSize);
            memcpy(&d2, snap2.data() + current, curSize);

            if (fabs((double)d1 - (double)d2) >= minDiff)
            {
                rmf_Log(rmf_Message, "Found a change!");
                MemoryRegionProperties nmrp = mrp;
                nmrp.relativeRegionSize     = curSize;
                nmrp.relativeRegionStart += current;
                data.push_back(std::move(nmrp));
            }
            current += curSize;
        }
        rmf_Log(rmf_Debug, "Found changes: " << data.size());
        return data;
    }

    template <Numeric NumType>
    std::vector<MemoryRegionProperties>
    _findUnchangedNumericCore(const CoreInputs& core,
                             const NumType&    maxDiff)
    {
        if (!core.snap1 || !core.snap2)
        {
            rmf_Log(rmf_Error, "Missing a snapshot!");
            return {};
        }
        const auto&                   snap1 = core.snap1.value();
        const auto&                   snap2 = core.snap2.value();
        const MemoryRegionProperties& mrp   = snap1.regionProperties;
        size_t numIters = snap1.size() / sizeof(NumType);
        // uintptr_t                     current  = 0;
        uintptr_t current = mrp.getActualRegionStart() -
            (mrp.getActualRegionStart() / sizeof(NumType)) *
                sizeof(NumType);
        rmf_Log(rmf_Debug, "Number of iterations required: " << numIters);

        std::vector<MemoryRegionProperties> data;
        for (size_t i = 0; i < numIters; ++i)
        {
            uintptr_t curSize = (i == numIters - 1) ?
                snap1.size() - current :
                sizeof(NumType);

            NumType   d1;
            NumType   d2;
            memcpy(&d1, snap1.data() + current, curSize);
            memcpy(&d2, snap2.data() + current, curSize);

            if (fabs((double)d1 - (double)d2) <= maxDiff)
            {
                rmf_Log(rmf_Message, "Found negligible change!");
                MemoryRegionProperties nmrp = mrp;
                nmrp.relativeRegionSize     = curSize;
                nmrp.relativeRegionStart += current;
                data.push_back(std::move(nmrp));
            }
            current += curSize;
        }
        rmf_Log(rmf_Debug, "Found changes: " << data.size());
        return data;
    }
}
#endif
