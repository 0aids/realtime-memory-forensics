#include "operations.hpp"
#include <cstring>
#include <iterator>
#include "logger.hpp"
#include "types.hpp"
// For all the operations

namespace rmf::op
{
    /******************************/
    /* Binary Snapshot Operations */
    /******************************/

    types::MemoryRegionPropertiesVec
    findChangedRegions(const types::MemorySnapshot& snap1,
                       const types::MemorySnapshot& snap2,
                       const uintptr_t&             compareSize)
    {
        auto                             span1 = snap1.getDataSpan();
        auto                             span2 = snap2.getDataSpan();
        auto&                            mrp   = snap1.getMrp();

        types::MemoryRegionPropertiesVec results;
        uintptr_t                        bytesCompared = 0;
        while (bytesCompared < span1.size())
        {
            uintptr_t toCompare =
                (span1.size() - bytesCompared > compareSize) ?
                compareSize :
                span1.size() - bytesCompared;

            if (memcmp(span1.data() + bytesCompared,
                       span2.data() + bytesCompared, toCompare))
            {
                rmf_Log(rmf_Debug, "Found difference!");
                if (!results.empty() &&
                    results.back().relativeEnd() ==
                        mrp.relativeRegionAddress + bytesCompared)
                {
                    results.back().relativeRegionSize += toCompare;
                }
                else
                {
                    types::MemoryRegionProperties toPush = mrp;
                    toPush.relativeRegionAddress += bytesCompared;
                    toPush.relativeRegionSize = toCompare;
                    results.push_back(toPush);
                }
            }
            bytesCompared += toCompare;
        }
        rmf_Log(
            rmf_Debug,
            "Number of changed regions found: " << results.size());
        return results;
    }

    types::MemoryRegionPropertiesVec
    findUnchangedRegions(const types::MemorySnapshot& snap1,
                         const types::MemorySnapshot& snap2,
                         const uintptr_t&             compareSize)
    {
        auto                             span1 = snap1.getDataSpan();
        auto                             span2 = snap2.getDataSpan();
        auto&                            mrp   = snap1.getMrp();

        types::MemoryRegionPropertiesVec results;
        uintptr_t                        bytesCompared = 0;
        while (bytesCompared < span1.size())
        {
            uintptr_t toCompare =
                (span1.size() - bytesCompared > compareSize) ?
                compareSize :
                span1.size() - bytesCompared;

            if (!memcmp(span1.data() + bytesCompared,
                        span2.data() + bytesCompared, toCompare))
            {
                rmf_Log(rmf_Debug, "Found No difference!");
                if (!results.empty() &&
                    results.back().relativeEnd() ==
                        mrp.relativeRegionAddress + bytesCompared)
                {
                    results.back().relativeRegionSize += toCompare;
                }
                else
                {
                    types::MemoryRegionProperties toPush = mrp;
                    toPush.relativeRegionAddress += bytesCompared;
                    toPush.relativeRegionSize = toCompare;
                    results.push_back(toPush);
                }
            }
            bytesCompared += toCompare;
        }
        rmf_Log(
            rmf_Debug,
            "Number of unchanged regions found: " << results.size());
        return results;
    }
    /*****************************/
    /* Unary Snapshot Operations */
    /*****************************/

    types::MemoryRegionPropertiesVec
    findString(const types::MemorySnapshot& snap1,
               const std::string_view&      str)
    {
        types::MemoryRegionPropertiesVec results;
        auto                             span = snap1.getDataSpan();

        for (uintptr_t i = 0; i + str.length() < span.size(); i++)
        {
            size_t count = 0;
            for (uintptr_t j = 0; j < str.length(); j++)
            {
                if (span[i + j] != str[j])
                {
                    break;
                }
                count++;
            }
            if (count == str.length())
            {
                auto mrp = snap1.getMrp();
                mrp.relativeRegionAddress += i;
                mrp.relativeRegionSize = str.length();
                results.push_back(mrp);
            }
        }
        return results;
    }

    types::MemoryRegionPropertiesVec
    findPointersToRegion(const types::MemorySnapshot&         snap1,
                         const types::MemoryRegionProperties& mrp)
    {
        return findNumeralWithinRange<uintptr_t>(
            snap1, mrp.TrueAddress(), mrp.TrueEnd());
    }

    types::MemoryRegionPropertiesVec findPointersToRegions(
        const types::MemorySnapshot&            snap1,
        const types::MemoryRegionPropertiesVec& mrps)
    {
        types::MemoryRegionPropertiesVec results;
        for (auto& mrp : mrps)
        {
            auto tempResults = findPointersToRegion(snap1, mrp);
            std::move(tempResults.begin(), tempResults.end(),
                      std::back_inserter(results));
        }
        return results;
    }
}
