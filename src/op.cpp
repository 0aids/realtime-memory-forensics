#include "rmf/op.hpp"
#include <cstring>

namespace rmf
{
    std::vector<Map> findChanged(MemoryRegionView in1, MemoryRegionView in2,
                                 uintptr_t compareSize)

    {
        std::vector<Map> results;
        uintptr_t        bytesCompared = 0;
        while (bytesCompared < in1.snap.data->size())
        {
            uintptr_t toCompare =
                (in1.snap.data->size() - bytesCompared > compareSize) ?
                    compareSize :
                    in1.snap.data->size() - bytesCompared;

            if (memcmp(in1.snap.data->data() + bytesCompared,
                       in2.snap.data->data() + bytesCompared, toCompare))
            {
                if (!results.empty() &&
                    results.back().rend() == in1.map.rAddr + bytesCompared)
                {
                    results.back().rSize += toCompare;
                }
                else
                {
                    Map map = in1.map;
                    map.rAddr += bytesCompared;
                    map.rSize = toCompare;
                    results.push_back(map);
                }
            }
            bytesCompared += toCompare;
        }
        return results;
    }

    std::vector<Map> findUnchanged(MemoryRegionView in1, MemoryRegionView in2,
                                   uintptr_t compareSize)

    {
        std::vector<Map> results;
        uintptr_t        bytesCompared = 0;
        while (bytesCompared < in1.snap.data->size())
        {
            uintptr_t toCompare =
                (in1.snap.data->size() - bytesCompared > compareSize) ?
                    compareSize :
                    in1.snap.data->size() - bytesCompared;

            if (!memcmp(in1.snap.data->data() + bytesCompared,
                        in2.snap.data->data() + bytesCompared, toCompare))
            {
                if (!results.empty() &&
                    results.back().rend() == in1.map.rAddr + bytesCompared)
                {
                    results.back().rSize += toCompare;
                }
                else
                {
                    Map toPush = in1.map;
                    toPush.rAddr += bytesCompared;
                    toPush.rSize = toCompare;
                    results.push_back(toPush);
                }
            }
            bytesCompared += toCompare;
        }
        return results;
    }

    std::vector<Map> findString(MemoryRegionView       in1,
                                const std::string_view str)
    {
        std::vector<Map> results;
        const char* head = reinterpret_cast<const char*>(in1.snap.data->data());
        const char* begin = head;
        const char* end   = head + in1.snap.data->size();

        while (head < end)
        {
            head =
                static_cast<const char*>(std::memchr(head, str[0], end - head));
            if (!head)
                break;

            if (std::memcmp(head, str.data(), str.size()) == 0)
            {
                Map map   = in1.map;
                map.rAddr = head - begin;
                map.rSize = str.size();
                results.push_back(map);
            }
            head++;
        }
        return results;
    }
}
