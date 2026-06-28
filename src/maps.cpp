#include "rmf/maps.hpp"
#include <cstddef>
namespace rmf
{
    bool Map::isLTSize(ptrdiff_t size) const
    {
        return rSize < size;
    }
    bool Map::isGTSize(ptrdiff_t size) const
    {
        return rSize > size;
    }
    bool Map::isEQSize(ptrdiff_t size) const
    {
        return rSize == size;
    }
    bool Map::isExactName(const std::string_view string) const
    {
        return string == *name;
    }
    bool Map::isSubName(const std::string_view string) const
    {
        return name->contains(string);
    }
    bool Map::isExactPerms(const std::string_view strperms) const
    {
        return Perms_Parse(strperms) == perms;
    }
    bool Map::isHavePerms(const std::string_view strperms) const
    {
        return (bool)(Perms_Parse(strperms) & perms);
    }
    bool Map::isNotHavePerms(const std::string_view strperms) const
    {
        return !(bool)(Perms_Parse(strperms) & perms);
    }

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

rmf::Perms operator|(rmf::Perms p1, rmf::Perms p2)
{
    return static_cast<rmf::Perms>(static_cast<uint8_t>(p1) |
                                   static_cast<uint8_t>(p2));
}
rmf::Perms& operator|=(rmf::Perms& p1, rmf::Perms p2)
{
    p1 = p1 | p2;
    return p1;
}
rmf::Perms operator&(rmf::Perms p1, rmf::Perms p2)
{
    return static_cast<rmf::Perms>(static_cast<uint8_t>(p1) &
                                   static_cast<uint8_t>(p2));
}
rmf::Perms& operator&=(rmf::Perms& p1, rmf::Perms p2)
{
    p1 = p1 & p2;
    return p1;
}
rmf::Perms operator^(rmf::Perms p1, rmf::Perms p2)
{
    return static_cast<rmf::Perms>(static_cast<uint8_t>(p1) ^
                                   static_cast<uint8_t>(p2));
}
rmf::Perms& operator^=(rmf::Perms& p1, rmf::Perms p2)
{
    p1 = p1 ^ p2;
    return p1;
}
rmf::Perms operator~(rmf::Perms p1)
{
    return static_cast<rmf::Perms>(~static_cast<uint8_t>(p1));
}
