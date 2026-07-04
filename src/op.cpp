#include "rmf/op.hpp"
#include <cstring>

namespace rmf
{
    std::vector<Map> findChanged(const Map& m1, const Snapshot& s1,
                                 const Map& m2, const Snapshot& s2,
                                 uintptr_t compareSize)
    {
        std::vector<Map> results;
        uintptr_t        bytesCompared = 0;
        while (bytesCompared < s1.size())
        {
            uintptr_t toCompare = (s1.size() - bytesCompared > compareSize) ?
                                      compareSize :
                                      s1.size() - bytesCompared;

            if (memcmp(s1.data() + bytesCompared, s2.data() + bytesCompared,
                       toCompare))
            {
                if (!results.empty() &&
                    results.back().rend() == m1.rAddr + bytesCompared)
                {
                    results.back().rSize += toCompare;
                }
                else
                {
                    Map map = m1;
                    map.rAddr += bytesCompared;
                    map.rSize = toCompare;
                    results.push_back(map);
                }
            }
            bytesCompared += toCompare;
        }
        return results;
    }

    std::vector<Map> findUnchanged(const Map& m1, const Snapshot& s1,
                                   const Map& m2, const Snapshot& s2,
                                   uintptr_t compareSize)

    {
        std::vector<Map> results;
        uintptr_t        bytesCompared = 0;
        while (bytesCompared < s1.size())
        {
            uintptr_t toCompare = (s1.size() - bytesCompared > compareSize) ?
                                      compareSize :
                                      s1.size() - bytesCompared;

            if (!memcmp(s1.data() + bytesCompared, s2.data() + bytesCompared,
                        toCompare))
            {
                if (!results.empty() &&
                    results.back().rend() == m1.rAddr + bytesCompared)
                {
                    results.back().rSize += toCompare;
                }
                else
                {
                    Map toPush = m1;
                    toPush.rAddr += bytesCompared;
                    toPush.rSize = toCompare;
                    results.push_back(toPush);
                }
            }
            bytesCompared += toCompare;
        }
        return results;
    }

    std::vector<Map> findString(const Map& m1, const Snapshot& s1,
                                const std::string_view str)
    {
        std::vector<Map> results;
        const char*      head  = reinterpret_cast<const char*>(s1.data());
        const char*      begin = head;
        const char*      end   = head + s1.size();

        while (head < end)
        {
            head =
                static_cast<const char*>(std::memchr(head, str[0], end - head));
            if (!head)
                break;

            if (std::memcmp(head, str.data(), str.size()) == 0)
            {
                Map map   = m1;
                map.rAddr = head - begin;
                map.rSize = str.size();
                results.push_back(map);
            }
            head++;
        }
        return results;
    }
}
