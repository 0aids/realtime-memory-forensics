#ifndef map_hpp_INCLUDED
#define map_hpp_INCLUDED
#include "rmf/mixin_helpers.hpp"
#include <cstddef>
#include <magic_enum/magic_enum.hpp>
#include <cstdint>
#include <memory>
#include <type_traits>
#include "rmf/utils/expect.hpp"
#include "rmf/node.hpp"
extern "C"
{
#include <fcntl.h>
#include <sys/uio.h>
}

namespace rmf
{
    enum class Perms : uint8_t;
}

enum class rmf::Perms : uint8_t
{
    None    = 0,
    Read    = 1,
    Write   = 2,
    Execute = 4,
    Shared  = 8, // Defaults to private.
    // Unimplemented, but may consider implementing.
    // MayRead = 16,
    // MayWrite = 32,
    // MayExecute = 64,
};

template <>
struct magic_enum::customize::enum_range<rmf::Perms>
{
    static constexpr bool is_flags = true;
};

// Detail related things.
#include "rmf/detail_map.tpp"

namespace rmf
{
    struct Map
    {
        Detail::MapData map;
        using usesMap = std::true_type;
        // Returns the address of the beginning of this region.
        constexpr uintptr_t RMF_MIXIN_METHOD(tbegin, () const);
        // Returns the address of the end of this region (exclusive).
        constexpr uintptr_t RMF_MIXIN_METHOD(tend, () const);
        // Returns the relative beginning (relative to the parent)
        constexpr uintptr_t RMF_MIXIN_METHOD(rbegin, () const);
        // Returns the relative beginning (relative to the parent)
        constexpr uintptr_t RMF_MIXIN_METHOD(rend, () const);
        // Returns the parent beginning
        constexpr uintptr_t RMF_MIXIN_METHOD(pbegin, () const);
        // Returns the parent beginning
        constexpr uintptr_t RMF_MIXIN_METHOD(pend, () const);

        template <typename T>
        bool                               RMF_MIXIN_METHOD(modify, ());

        std::shared_ptr<const std::string> RMF_MIXIN_METHOD(getName, ());

                                           operator std::string();

        struct VecOp
        {
            // using InnerType = BaseVec::InnerType;

            template <NodeWithFeatures<Map> Self>
            Self minSize(this const Self& self, size_t);
            template <NodeWithFeatures<Map> Self>
            Self maxSize(this const Self& self, size_t);
            template <NodeWithFeatures<Map> Self>
            Self chunkify(this const Self& self, size_t chunkSize,
                          size_t overlapSize);

            // TODO: Move naming filters to regex.

            template <NodeWithFeatures<Map> Self>
            Self exactName(this const Self& self, const std::string_view);
            template <NodeWithFeatures<Map> Self>
            Self subName(this const Self& self, const std::string_view);

            template <NodeWithFeatures<Map> Self>
            Self exactPerms(this const Self&       self,
                            const std::string_view perms);
            template <NodeWithFeatures<Map> Self>
            Self hasPerms(this const Self& self, const std::string_view perms);
            template <NodeWithFeatures<Map> Self>
            Self notPerms(this const Self& self, const std::string_view perms);
            template <NodeWithFeatures<Map> Self>
            Self active(this const Self& self, pid_t pid);
        };
    };
}

namespace rmf
{
    using namespace magic_enum::bitwise_operators;
    // Returns the address of the beginning of this region.
    constexpr uintptr_t Map::tbegin() const
    {
        return map.parentAddress + map.relativeAddress;
    }
    // Returns the address of the end of this region (exclusive). const
    constexpr uintptr_t Map::tend() const
    {
        return map.parentAddress + map.relativeAddress + map.relativeSize;
    }
    // Returns the relative beginning (relative to the parent) const
    constexpr uintptr_t Map::rbegin() const
    {
        return map.relativeAddress;
    }
    // Returns the relative beginning (relative to the parent) const
    constexpr uintptr_t Map::rend() const
    {
        return map.relativeAddress + map.relativeSize;
    }
    // Returns the parent beginning
    constexpr uintptr_t Map::pbegin() const
    {
        return map.parentAddress;
    }
    // Returns the parent beginning
    constexpr uintptr_t Map::pend() const
    {
        return map.parentAddress + map.parentSize;
    }
    // using InnerType = BaseVec::InnerType;

    template <NodeWithFeatures<Map> Self>
    Self Map::VecOp::minSize(this const Self& self, size_t minSize)
    {
        Self rl;
        for (size_t i = 0; i < self.size(); i++)
        {
            if (self.at(i).map.relativeSize >= minSize)
            {
                rl.push_back(self.at(i));
            }
        }

        return rl;
    }

    template <NodeWithFeatures<Map> Self>
    Self Map::VecOp::maxSize(this const Self& self, size_t maxSize)
    {
        Self rl;
        for (size_t i = 0; i < self.size(); i++)
        {
            if (self.at(i).map.relativeSize <= maxSize)
            {
                rl.push_back(self.at(i));
            }
        }

        return rl;
    }
    template <NodeWithFeatures<Map> Self>
    Self Map::VecOp::chunkify(this const Self& self, size_t chunkSize,
                              size_t overlapSize)
    {
        Self      res;
        uintptr_t overallSize = 0;
        for (const auto& mrp : self)
        {
            overallSize += mrp.map.relativeSize;
        }

        res.reserve(overallSize / chunkSize + 1);

        for (const auto& mrp : self)
        {
            uintptr_t       ptrHead = mrp.map.relativeAddress;
            const uintptr_t end     = mrp.map.relativeEnd();

            while (ptrHead < end)
            {
                const uintptr_t actualChunkSize =
                    (end - ptrHead > chunkSize) ? chunkSize : end - ptrHead;
                res.push_back(mrp);
                res.back().map.relativeSize    = actualChunkSize;
                res.back().map.relativeAddress = ptrHead;
                ptrHead += actualChunkSize;
                if (ptrHead >= end)
                    break;
                ptrHead -= overlapSize;
            }
        }
        rmf_Debug("Total size: {:08x}", overallSize);
        rmf_Debug("Broken into: {} chunks", res.size());
        return res;
    }

    template <NodeWithFeatures<Map> Self>
    Self Map::VecOp::exactName(this const Self&       self,
                               const std::string_view string)
    {
        Self rl;
        for (size_t i = 0; i < self.size(); i++)
        {
            if (*(self.at(i).map.regionName_sp) == string)
            {
                rl.push_back(self.at(i));
            }
        }

        return rl;
    }
    template <NodeWithFeatures<Map> Self>
    Self Map::VecOp::subName(this const Self&       self,
                             const std::string_view string)
    {
        Self rl;
        for (size_t i = 0; i < self.size(); i++)
        {
            if (self.at(i).map.regionName_sp->contains(string))
            {
                rl.push_back(self.at(i));
            }
        }

        return rl;
    }

    template <NodeWithFeatures<Map> Self>
    Self Map::VecOp::exactPerms(this const Self&       self,
                                const std::string_view perms)
    {
        Perms permsToMatch = Detail::parsePerms(perms);
        Self  rl;
        for (size_t i = 0; i < self.size(); i++)
        {
            const Perms p = self.at(i).map.perms;
            if (p == permsToMatch)
            {
                rl.push_back(self.at(i));
            }
        }
        return rl;
    }
    template <NodeWithFeatures<Map> Self>
    Self Map::VecOp::hasPerms(this const Self&       self,
                              const std::string_view perms)
    {
        Perms permsToMatch = Detail::parsePerms(perms);
        Self  rl;
        for (size_t i = 0; i < self.size(); i++)
        {
            const Perms p = self.at(i).map.perms;
            if ((p & permsToMatch) == permsToMatch)
            {
                rl.push_back(self.at(i));
            }
        }
        return rl;
    }
    template <NodeWithFeatures<Map> Self>
    Self Map::VecOp::notPerms(this const Self&       self,
                              const std::string_view perms)
    {
        Perms permsToMatch = Detail::parsePerms(perms);
        Self  rl;
        for (size_t i = 0; i < self.size(); i++)
        {
            const Perms p = self.at(i).map.perms;
            if ((p & permsToMatch) != permsToMatch)
            {
                rl.push_back(self.at(i));
            }
        }
        return rl;
    }

    template <NodeWithFeatures<Map> Self>
    Self Map::VecOp::active(this const Self& self, pid_t pid)
    {
        if (self.size() == 0)
        {
            rmf_Warning("Given an empty types::MemoryRegionPropertiesVec!!!");
            return {};
        }
        Self              regions;
        long              pageSize    = sysconf(_SC_PAGE_SIZE);
        const std::string pagemapPath = std::format("/proc/{}/pagemap", pid);
        rmf_Info("Reading pagemap: {}", pagemapPath);
        int fd = open(pagemapPath.c_str(), O_RDONLY);
        if (fd < 0)
        {
            rmf_Error("Failed to open the pageMap!!!!");
            return {};
        }
        static constexpr uint64_t ACTIVE_BIT = (1ULL << 63);
        for (const auto& mrp : self)
        {
            for (uintptr_t addr = mrp.tbegin();
                 addr < mrp.tbegin() + mrp.map.relativeSize; addr += pageSize)
            {
                // Multiply by 8 because each 8 byte chunk represents a page.
                uintptr_t offset = (addr / pageSize) * 8;
                if (lseek(fd, offset, SEEK_SET) == (off_t)-1)
                {
                    rmf_Error("Failed to seek the pagemap!");
                    perror("failed to seek pagemap");
                    continue;
                }

                uint64_t entry;
                ssize_t  readResult = read(fd, &entry, 8);
                if (readResult == (ssize_t)-1)
                {
                    rmf_Error("Failed to READ the pagemap!");
                    perror("failed to read pagemap");
                    continue;
                }

                else if (readResult < (ssize_t)sizeof(entry))
                {
                    rmf_Warning("Read only {}/{} bytes!", readResult,
                                sizeof(entry));
                    continue;
                }

                if (entry & ACTIVE_BIT)
                {
                    if (regions.size() > 0 &&
                        regions.back().rend() == addr - mrp.map.parentAddress)
                    {
                        regions.back().map.relativeSize += pageSize;
                    }
                    else
                    {
                        typename Self::InnerType newMrp = mrp;
                        newMrp.map.relativeSize         = pageSize;
                        newMrp.map.relativeAddress =
                            addr - mrp.map.parentAddress;
                        regions.push_back(newMrp);
                    }
                }
            }
        }
        close(fd);
        rmf_Info("Active regions: {}", regions.size());
        return regions;
    }
}
#endif // map_hpp_INCLUDED
