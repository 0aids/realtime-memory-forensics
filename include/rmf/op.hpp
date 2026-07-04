#pragma once
#include "rmf/config.hpp"
#include "rmf/snapshots.hpp"
#include "rmf/maps.hpp"

#include <cstring>

#include <cassert>

namespace rmf
{
    std::vector<Map> findChanged(const Map& m1, const Snapshot& s1,
                                 const Map& m2, const Snapshot& s2,
                                 uintptr_t compareSize);

    std::vector<Map> findUnchanged(const Map& m1, const Snapshot& s1,
                                   const Map& m2, const Snapshot& s2,
                                   uintptr_t compareSize);

    template <Numeric N>
    std::vector<Map> findNumChanged(const Map& m1, const Snapshot& s1,
                                    const Map& m2, const Snapshot& s2,
                                    N minChangeRequired);

    template <Numeric N>
    std::vector<Map> findNumUnchanged(const Map& m1, const Snapshot& s1,
                                      const Map& m2, const Snapshot& s2,
                                      N maxChangeRequired);

    std::vector<Map> findString(const Map& m1, const Snapshot& s1,
                                const std::string_view str);

    template <Numeric N>
    std::vector<Map> findNumExact(const Map& m1, const Snapshot& s1, N num);

    template <Numeric N>
    std::vector<Map> findNumInRange(const Map& m1, const Snapshot& s1, N min,
                                    N max);
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
    std::vector<Map> findNumChanged(const Map& m1, const Snapshot& s1,
                                    const Map& m2, const Snapshot& s2,
                                    N minChangeRequired)

    {
        std::vector<Map> results;
        constexpr size_t alignment = alignof(N);
        constexpr size_t size      = sizeof(N);

        // Prealign to the next available slot.
        uintptr_t bytesCompared = Dtl::prealign(m1, alignment);

        // Ensure we don't read out of bounds
        while (bytesCompared + size <= s1.size())
        {
            N value1;
            memcpy(&value1, s1.data() + bytesCompared, size);
            N value2;
            memcpy(&value2, s2.data() + bytesCompared, size);

            N diff = value2 - value1;

            if (diff >= minChangeRequired)
            {
                Map newnode = m1;
                newnode.rAddr += bytesCompared;
                newnode.rSize = size;
                results.push_back(newnode);
            }

            bytesCompared += alignment;
        }
        return results;
    }

    template <Numeric N>
    std::vector<Map> findNumUnchanged(const Map& m1, const Snapshot& s1,
                                      const Map& m2, const Snapshot& s2,
                                      N maxChangeRequired)
    {
        std::vector<Map> results;
        constexpr size_t alignment = alignof(N);
        constexpr size_t size      = sizeof(N);

        // Prealign to the next available slot.
        uintptr_t bytesCompared = Dtl::prealign(m1, alignment);

        // Ensure we don't read out of bounds
        while (bytesCompared + size <= s1.size())
        {
            N value1;
            memcpy(&value1, s1.data() + bytesCompared, size);
            N value2;
            memcpy(&value2, s2.data() + bytesCompared, size);

            N diff = value2 - value1;

            if (diff <= maxChangeRequired)
            {
                Map map = m1;
                map.rAddr += bytesCompared;
                map.rSize = size;
                results.push_back(map);
            }

            bytesCompared += alignment;
        }
        return results;
    }
    template <Numeric N>
    std::vector<Map> findNumExact(const Map& m1, const Snapshot& s1, N num)
    {
        std::vector<Map> results;
        constexpr size_t alignment     = alignof(N);
        constexpr size_t size          = sizeof(N);
        uintptr_t        bytesCompared = Dtl::prealign(m1, alignment);

        while (bytesCompared + size <= s1.size())
        {
            N value;
            memcpy(&value, s1.data() + bytesCompared, size);
            if (value == num)
            {
                Map map = m1;
                map.rAddr += bytesCompared;
                map.rSize = size;
                results.push_back(map);
            }
            bytesCompared += alignment;
        }
        return results;
    }

    template <Numeric N>
    std::vector<Map> findNumInRange(const Map& m1, const Snapshot& s1, N min,
                                    N max)
    {
        std::vector<Map> results;
        constexpr size_t alignment     = alignof(N);
        constexpr size_t size          = sizeof(N);
        uintptr_t        bytesCompared = Dtl::prealign(m1, alignment);

        while (bytesCompared + size <= s1.size())
        {
            N value;
            memcpy(&value, s1.data() + bytesCompared, size);
            if (min <= value && value <= max)
            {
                Map map = m1;
                map.rAddr += bytesCompared;
                map.rSize = size;
                results.push_back(map);
            }
            bytesCompared += alignment;
        }
        return results;
    }
} // namespace rmf
