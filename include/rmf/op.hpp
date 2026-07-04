#pragma once
#include "rmf/config.hpp"
#include "rmf/snapshots.hpp"
#include "rmf/memory_region.hpp"
#include <cstring>

#include <cassert>

namespace rmf
{
    std::vector<Map> findChanged(MemoryRegionView in1, MemoryRegionView in2,
                                 uintptr_t compareSize);

    std::vector<Map> findUnchanged(MemoryRegionView in1, MemoryRegionView in2,
                                   uintptr_t compareSize);

    template <Numeric N>
    std::vector<Map> findNumChanged(MemoryRegionView in1, MemoryRegionView in2,
                                    N minChangeRequired);

    template <Numeric N>
    std::vector<Map> findNumUnchanged(MemoryRegionView in1,
                                      MemoryRegionView in2,
                                      N                maxChangeRequired);

    std::vector<Map> findString(MemoryRegionView       in1,
                                const std::string_view str);

    template <Numeric N>
    std::vector<Map> findNumExact(MemoryRegionView in1, N num);

    template <Numeric N>
    std::vector<Map> findNumInRange(MemoryRegionView in1, N min, N max);
} // namespace rmf

namespace rmf
{
    namespace Dtl
    {
        constexpr ptrdiff_t prealign(const Map& map, ptrdiff_t alignment)
        {
            if ((map.tbegin() / alignment) * alignment < map.tbegin())
            {
                return alignment + (map.tbegin() / alignment * alignment) -
                       map.tbegin();
            }
            return 0;
        }
    }
    template <Numeric N>
    std::vector<Map> findNumChanged(MemoryRegionView in1, MemoryRegionView in2,
                                    N minChangeRequired)

    {
        std::vector<Map> results;
        constexpr size_t alignment = alignof(N);
        constexpr size_t size      = sizeof(N);

        // Prealign to the next available slot.
        uintptr_t bytesCompared = Dtl::prealign(in1.map, alignment);

        // Ensure we don't read out of bounds
        while (bytesCompared + size <= in1.snap.data->size())
        {
            N value1;
            memcpy(&value1, in1.snap.data->data() + bytesCompared, size);
            N value2;
            memcpy(&value2, in2.snap.data->data() + bytesCompared, size);

            N diff = value2 - value1;

            if (diff >= minChangeRequired)
            {
                Map newnode = in1.map;
                newnode.rAddr += bytesCompared;
                newnode.rSize = size;
                results.push_back(newnode);
            }

            bytesCompared += alignment;
        }
        return results;
    }

    template <Numeric N>
    std::vector<Map> findNumUnchanged(MemoryRegionView in1,
                                      MemoryRegionView in2, N maxChangeRequired)
    {
        std::vector<Map> results;
        constexpr size_t alignment = alignof(N);
        constexpr size_t size      = sizeof(N);

        // Prealign to the next available slot.
        uintptr_t bytesCompared = Dtl::prealign(in1.map, alignment);

        // Ensure we don't read out of bounds
        while (bytesCompared + size <= in1.snap.data->size())
        {
            N value1;
            memcpy(&value1, in1.snap.data->data() + bytesCompared, size);
            N value2;
            memcpy(&value2, in2.snap.data->data() + bytesCompared, size);

            N diff = value2 - value1;

            if (diff <= maxChangeRequired)
            {
                Map map = in1.map;
                map.rAddr += bytesCompared;
                map.rSize = size;
                results.push_back(map);
            }

            bytesCompared += alignment;
        }
        return results;
    }
    template <Numeric N>
    std::vector<Map> findNumExact(MemoryRegionView in1, N num)
    {
        std::vector<Map> results;
        constexpr size_t alignment     = alignof(N);
        constexpr size_t size          = sizeof(N);
        uintptr_t        bytesCompared = Dtl::prealign(in1.map, alignment);

        while (bytesCompared + size <= in1.snap.data->size())
        {
            N value;
            memcpy(&value, in1.snap.data->data() + bytesCompared, size);
            if (value == num)
            {
                Map map = in1.map;
                map.rAddr += bytesCompared;
                map.rSize = size;
                results.push_back(map);
            }
            bytesCompared += alignment;
        }
        return results;
    }

    template <Numeric N>
    std::vector<Map> findNumInRange(MemoryRegionView in1, N min, N max)
    {
        std::vector<Map> results;
        constexpr size_t alignment     = alignof(N);
        constexpr size_t size          = sizeof(N);
        uintptr_t        bytesCompared = Dtl::prealign(in1.map, alignment);

        while (bytesCompared + size <= in1.snap.data->size())
        {
            N value;
            memcpy(&value, in1.snap.data->data() + bytesCompared, size);
            if (min <= value && value <= max)
            {
                Map map = in1.map;
                map.rAddr += bytesCompared;
                map.rSize = size;
                results.push_back(map);
            }
            bytesCompared += alignment;
        }
        return results;
    }
} // namespace rmf
