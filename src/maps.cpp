#include "rmf/maps.hpp"
#include <cstddef>
namespace rmf
{

    std::vector<Map> Map::chunkify(ptrdiff_t chunkSize, ptrdiff_t overlap) const
    {
        std::vector<Map> res;

        res.reserve(rSize / chunkSize + 1);

        uintptr_t       ptrHead = rAddr;
        const uintptr_t end     = rend();

        while (ptrHead < end)
        {
            const uintptr_t actualChunkSize =
                (static_cast<ptrdiff_t>(end - ptrHead) > chunkSize) ?
                    chunkSize :
                    end - ptrHead;
            res.push_back(*this);
            res.back().rSize = actualChunkSize;
            res.back().rAddr = ptrHead;
            ptrHead += actualChunkSize;
            if (ptrHead >= end)
                break;
            ptrHead -= overlap;
        }
        return res;
    }
} // namespace rmf
