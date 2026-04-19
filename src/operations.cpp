#include "operations.hpp"
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <unordered_map>
#include "logger.hpp"
#include "types.hpp"
#include "utils.hpp"
// For all the operations

namespace rmf::op
{
    using namespace rmf::types;
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
               const std::string_view       str)
    {
        types::MemoryRegionPropertiesVec results;
        auto                             span = snap1.getDataSpan();
        const char* head = reinterpret_cast<const char*>(span.data());
        const char* begin = head;
        const char* end   = head + span.size();

        while (head < end)
        {
            head = static_cast<const char*>(
                std::memchr(head, str[0], end - head));
            if (!head)
                break;

            if (std::memcmp(head, str.data(), str.size()) == 0)
            {
                auto mrp                  = snap1.getMrp();
                mrp.relativeRegionAddress = head - begin;
                mrp.relativeRegionSize    = str.size();
                results.push_back(mrp);
            }
            head++;
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

    types::MemoryRegionPropertiesVec
    findPointersToRegionsRestructured(
        const types::MemorySnapshot&            snap1,
        const types::MemoryRegionPropertiesVec& mrps,
        const types::MrpRestructure&            restructure)
    {
        types::MemoryRegionPropertiesVec results;
        for (auto& mrp : mrps)
        {
            auto tempResults = findPointersToRegion(
                snap1, utils::RestructureMrp(mrp, restructure));
            std::move(tempResults.begin(), tempResults.end(),
                      std::back_inserter(results));
        }
        return results;
    }

    types::MemoryRegionPropertiesVec findPointersToRegionRestructured(
        const types::MemorySnapshot&         snap1,
        const types::MemoryRegionProperties& mrp,
        const types::MrpRestructure&         restructure)
    {
        auto newMrp = utils::RestructureMrp(mrp, restructure);
        return findPointersToRegion(snap1, newMrp);
    }

    types::MapifiedSnap mapifySnap(const types::MemorySnapshot& snap)
    {
        types::MapifiedSnap pointers;
        pointers.sourceMrp = snap.getMrp();

        const auto mrp  = snap.getMrp();
        const auto data = snap.getDataSpan();

        ptrdiff_t  head =
            sizeof(uintptr_t) - mrp.TrueAddress() % sizeof(uintptr_t);

        pointers.sourceTargetPairs.reserve(
            (mrp.relativeRegionSize - head) / sizeof(uintptr_t));

        const uintptr_t* ptr_data =
            reinterpret_cast<const uintptr_t*>(data.data() + head);
        size_t count =
            (mrp.relativeRegionSize - head) / sizeof(uintptr_t);

        for (size_t i = 0; i < count; ++i)
        {
            uintptr_t target = ptr_data[i];
            uintptr_t source =
                head + (i * sizeof(uintptr_t)) + mrp.TrueAddress();
            pointers.sourceTargetPairs.emplace_back(source, target);
        }

        // Sort the flat array by target address for fast binary searching later
        std::sort(pointers.sourceTargetPairs.begin(),
                  pointers.sourceTargetPairs.end(),
                  [](const auto& a, const auto& b)
                  { return a.target < b.target; });

        return pointers;
    }

    // Search if a regions' true address lies within some other region.
    std::vector<
        std::pair<MemoryRegionProperties, MemoryRegionProperties>>
    findSourcesOfTargetRegions(
        const MapifiedSnap&              mapsnap,
        const MemoryRegionPropertiesVec& regions,
        const MrpRestructure&            mrpRestructure)
    {
        std::vector<
            std::pair<MemoryRegionProperties, MemoryRegionProperties>>
                    result;
        const auto& sourceTargetPairs = mapsnap.sourceTargetPairs;

        for (auto target : regions)
        {
            target = utils::RestructureMrp(target, mrpRestructure);

            // Binary search the flat vector by the target
            auto sourcesBottom = std::lower_bound(
                sourceTargetPairs.begin(), sourceTargetPairs.end(),
                target.TrueAddress(),
                [](const SourceTargetPointerPair& element,
                   uintptr_t val) { return element.target < val; });

            auto sourcesTop = std::upper_bound(
                sourceTargetPairs.begin(), sourceTargetPairs.end(),
                target.TrueEnd() - 1,
                [](uintptr_t                      val,
                   const SourceTargetPointerPair& element)
                { return element.target > val; });

            for (auto it = sourcesBottom; it != sourcesTop; ++it)
            {
                MemoryRegionProperties newSourceMrp =
                    mapsnap.sourceMrp;
                newSourceMrp.relativeRegionAddress =
                    it->source - newSourceMrp.parentRegionAddress;
                newSourceMrp.relativeRegionSize = sizeof(uintptr_t);
                result.emplace_back(newSourceMrp, target);
            }
        }
        return result;
    }
}
