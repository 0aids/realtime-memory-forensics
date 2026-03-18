#ifndef operations_hpp_INCLUDED
#define operations_hpp_INCLUDED
#include "types.hpp"
#include "logger.hpp"
#include <cstring>

namespace rmf::op
{
    /******************************/
    /* Binary Snapshot Operations */
    /******************************/
    types::MemoryRegionPropertiesVec
    findChangedRegions(const types::MemorySnapshot& snap1,
                       const types::MemorySnapshot& snap2,
                       const uintptr_t&             compareSize);
    types::MemoryRegionPropertiesVec
    findUnchangedRegions(const types::MemorySnapshot& snap1,
                         const types::MemorySnapshot& snap2,
                         const uintptr_t&             compareSize);

    // Difference is calculated as snap2 - snap1
    // Inclusive.
    template <types::Numeral N>
    types::MemoryRegionPropertiesVec
    findNumericChanged(const types::MemorySnapshot& snap1,
                       const types::MemorySnapshot& snap2,
                       const N&                     minDifference)
    {
        auto                             span1 = snap1.getDataSpan();
        auto                             span2 = snap2.getDataSpan();
        auto&                            mrp   = snap1.getMrp();
        types::MemoryRegionPropertiesVec results;
        const size_t                     alignment = alignof(N);
        const size_t                     size      = sizeof(N);

        // Prealign to the next available slot.
        uintptr_t bytesCompared = 0;
        if ((mrp.TrueAddress() / alignment) * alignment <
            mrp.TrueAddress())
        {
            bytesCompared += alignment +
                (mrp.TrueAddress() / alignment * alignment) -
                mrp.TrueAddress();
        }

        // Ensure we don't read out of bounds
        while (bytesCompared + size < span1.size())
        {
            N value1;
            memcpy(&value1, span1.data() + bytesCompared, size);
            N value2;
            memcpy(&value2, span2.data() + bytesCompared, size);

            N diff = value2 - value1;

            if (diff >= minDifference)
            {
                auto newmrp = mrp;
                newmrp.relativeRegionAddress += bytesCompared;
                newmrp.relativeRegionSize = size;
                results.push_back(newmrp);
            }

            bytesCompared += alignment;
        }

        rmf_Log(rmf_Debug,
                "Number of numerically changed regions found: "
                    << results.size());
        return results;
    }

    // Inclusive.
    template <types::Numeral N>
    types::MemoryRegionPropertiesVec
    findNumericUnchanged(const types::MemorySnapshot& snap1,
                         const types::MemorySnapshot& snap2,
                         const N&                     maxDifference)
    {
        auto                             span1 = snap1.getDataSpan();
        auto                             span2 = snap2.getDataSpan();
        auto&                            mrp   = snap1.getMrp();
        types::MemoryRegionPropertiesVec results;
        const size_t                     alignment = alignof(N);
        const size_t                     size      = sizeof(N);

        // Prealign to the next available slot.
        uintptr_t bytesCompared = 0;
        if ((mrp.TrueAddress() / alignment) * alignment <
            mrp.TrueAddress())
        {
            bytesCompared += alignment +
                (mrp.TrueAddress() / alignment * alignment) -
                mrp.TrueAddress();
        }

        // Ensure we don't read out of bounds
        while (bytesCompared + size < span1.size())
        {
            N value1;
            memcpy(&value1, span1.data() + bytesCompared, size);
            N value2;
            memcpy(&value2, span2.data() + bytesCompared, size);

            N diff = value2 - value1;

            if (diff <= maxDifference)
            {
                auto newmrp = mrp;
                newmrp.relativeRegionAddress += bytesCompared;
                newmrp.relativeRegionSize = size;
                results.push_back(newmrp);
            }

            bytesCompared += alignment;
        }

        rmf_Log(rmf_Debug,
                "Number of numerically changed regions found: "
                    << results.size());
        return results;
    }
    /*****************************/
    /* Unary Snapshot Operations */
    /*****************************/

    types::MemoryRegionPropertiesVec
    findString(const types::MemorySnapshot& snap1,
               const std::string_view&      str);

    template <types::Numeral N>
    types::MemoryRegionPropertiesVec
    findNumeralExact(const types::MemorySnapshot& snap1,
                     const N                      number)
    {
        auto                             span = snap1.getDataSpan();
        auto&                            mrp  = snap1.getMrp();
        types::MemoryRegionPropertiesVec results;
        const size_t                     alignment     = alignof(N);
        const size_t                     size          = sizeof(N);
        uintptr_t                        bytesCompared = 0;
        if ((mrp.TrueAddress() / alignment) * alignment <
            mrp.TrueAddress())
        {
            bytesCompared += alignment +
                (mrp.TrueAddress() / alignment * alignment) -
                mrp.TrueAddress();
        }

        while (bytesCompared + size < span.size())
        {
            N value;
            memcpy(&value, span.data() + bytesCompared, size);
            if (value == number)
            {
                auto newmrp = mrp;
                newmrp.relativeRegionAddress += bytesCompared;
                newmrp.relativeRegionSize = size;
                results.push_back(newmrp);
                rmf_Log(
                    rmf_Debug,
                    "Found value at address: " << newmrp.toString());
            }
            bytesCompared += alignment;
        }
        return results;
    }

    // Inclusive.
    template <types::Numeral N>
    types::MemoryRegionPropertiesVec
    findNumeralWithinRange(const types::MemorySnapshot& snap1,
                           const N& min, const N& max)
    {
        auto                             span = snap1.getDataSpan();
        auto&                            mrp  = snap1.getMrp();
        types::MemoryRegionPropertiesVec results;
        const size_t                     alignment     = alignof(N);
        const size_t                     size          = sizeof(N);
        uintptr_t                        bytesCompared = 0;
        if ((mrp.TrueAddress() / alignment) * alignment <
            mrp.TrueAddress())
        {
            bytesCompared += alignment +
                (mrp.TrueAddress() / alignment * alignment) -
                mrp.TrueAddress();
        }

        while (bytesCompared + size < span.size())
        {
            N value;
            memcpy(&value, span.data() + bytesCompared, size);
            if (min <= value && value <= max)
            {
                auto newmrp = mrp;
                newmrp.relativeRegionAddress += bytesCompared;
                newmrp.relativeRegionSize = size;
                results.push_back(newmrp);
            }
            bytesCompared += alignment;
        }
        return results;
    }

    types::MemoryRegionPropertiesVec
    findPointersToRegion(const types::MemorySnapshot&         snap1,
                         const types::MemoryRegionProperties& mrp);

    types::MemoryRegionPropertiesVec findPointersToRegions(
        const types::MemorySnapshot&            snap1,
        const types::MemoryRegionPropertiesVec& mrps);

    /*
     * Find struct using pointers.
     * Dunno how to implement this at the moment, but it's something to consider.
     * */
}

#endif
