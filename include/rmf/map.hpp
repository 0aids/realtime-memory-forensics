#ifndef map_hpp_INCLUDED
#define map_hpp_INCLUDED
#include <cstddef>
#include <magic_enum/magic_enum.hpp>
#include <cstdint>
#include <memory>
#include <type_traits>
#include "rmf/utils/str.hpp"
#include "rmf/node.hpp"
extern "C"
{
#include <fcntl.h>
#include <sys/uio.h>
}

namespace RealtimeMemoryForensics
{
    enum class Perms : uint8_t;
}

enum class RealtimeMemoryForensics::Perms : uint8_t
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
struct magic_enum::customize::enum_range<
    RealtimeMemoryForensics::Perms>
{
    static constexpr bool is_flags = true;
};

namespace RealtimeMemoryForensics
{
    namespace Detail
    {
        Perms parsePerms(const std::string_view perms);
        struct MapData
        {
            static std::shared_ptr<const std::string> defaultName;
            // Default values for safety
            uintptr_t                          parentAddress   = 0;
            uintptr_t                          parentSize      = 0;
            ptrdiff_t                          relativeAddress = 0;
            ptrdiff_t                          relativeSize    = 0;
            std::shared_ptr<const std::string> regionName_sp =
                defaultName;
            Perms perms = Perms::None;
            bool  operator==(const MapData& other) const = default;
        };
    }

    struct Map
    {
        Detail::MapData map;
        using usesMap = std::true_type;

        Perms perms = Perms::None;
        // Returns the address of the beginning of this region.
        template <class Self>
        constexpr uintptr_t tbegin(this const Self& self);
        // Returns the address of the end of this region (exclusive).
        template <class Self>
        constexpr uintptr_t tend(this const Self& self);
        // Returns the relative beginning (relative to the parent)
        template <class Self>
        constexpr uintptr_t rbegin(this const Self& self);
        // Returns the relative beginning (relative to the parent)
        template <class Self>
        constexpr uintptr_t rend(this const Self& self);
        // Returns the parent beginning
        template <class Self>
        constexpr uintptr_t pbegin(this const Self& self);
        // Returns the parent beginning
        template <class Self>
        constexpr uintptr_t pend(this const Self& self);
        template <class Self>
        operator std::string(this const Self& self);

        struct VecOp
        {
            // using InnerType = BaseVec::InnerType;

            template <class Self>
            Self minSize(this const Self& self, size_t);
            template <class Self>
            Self maxSize(this const Self& self, size_t);
            template <class Self>
            Self chunkify(this const Self& self, size_t chunkSize,
                          size_t overlapSize);

            // TODO: Move naming filters to regex.

            template <class Self>
            Self exactName(this const Self& self,
                           const std::string_view);
            template <class Self>
            Self subName(this const Self& self,
                         const std::string_view);

            template <class Self>
            Self exactPerms(this const Self&       self,
                            const std::string_view perms);
            template <class Self>
            Self hasPerms(this const Self&       self,
                          const std::string_view perms);
            template <class Self>
            Self notPerms(this const Self&       self,
                          const std::string_view perms);
            template <class Self>
            Self active(this const Self& self, pid_t pid);
        };
    };
}

namespace RealtimeMemoryForensics
{
    using namespace magic_enum::bitwise_operators;
    // Returns the address of the beginning of this region.
    template <class Self>
    constexpr uintptr_t Map::tbegin(this const Self& self)
    { return self.map.parentAddress + self.map.relativeAddress; }
    // Returns the address of the end of this region (exclusive).
    template <class Self>
    constexpr uintptr_t Map::tend(this const Self& self)
    {
        return self.map.parentAddress + self.map.relativeAddress +
            self.map.relativeSize;
    }
    // Returns the relative beginning (relative to the parent)
    template <class Self>
    constexpr uintptr_t Map::rbegin(this const Self& self)
    { return self.map.relativeAddress; }
    // Returns the relative beginning (relative to the parent)
    template <class Self>
    constexpr uintptr_t Map::rend(this const Self& self)
    { return self.map.relativeAddress + self.map.relativeSize; }
    // Debugging use?
    template <class Self>
    Map::operator std::string(this const Self& self)
    {
        using namespace RealtimeMemoryForensics::Utils::Literals;
        return "[{}] - Parent Region: [{}, {}) Actual Region: [{}, {})"_f
            .fmt(*self.map.regionName_sp, self.pbegin(), self.pend(),
                 self.tbegin(), self.tend());
    }
    // Returns the parent beginning
    template <class Self>
    constexpr uintptr_t Map::pbegin(this const Self& self)
    { return self.map.parentAddress; }
    // Returns the parent beginning
    template <class Self>
    constexpr uintptr_t Map::pend(this const Self& self)
    { return self.map.parentAddress + self.map.parentSize; }
    // using InnerType = BaseVec::InnerType;

    template <class Self>
    Self Map::VecOp::minSize(this const Self& self, size_t minSize)
    {
        Self rl;
        for (size_t i = 0; i < self.size(); i++)
        {
            if (self.at(i).relativeRegionSize >= minSize)
            {
                rl.push_back(self.at(i));
            }
        }

        return rl;
    }

    template <class Self>
    Self Map::VecOp::maxSize(this const Self& self, size_t maxSize)
    {
        Self rl;
        for (size_t i = 0; i < self.size(); i++)
        {
            if (self.at(i).relativeRegionSize <= maxSize)
            {
                rl.push_back(self.at(i));
            }
        }

        return rl;
    }
    template <class Self>
    Self Map::VecOp::chunkify(this const Self& self, size_t chunkSize,
                              size_t overlapSize)
    {
        Self      res;
        uintptr_t overallSize = 0;
        for (const auto& mrp : self)
        {
            overallSize += mrp.relativeRegionSize;
        }

        res.reserve(overallSize / chunkSize + 1);

        for (const auto& mrp : self)
        {
            uintptr_t       ptrHead = mrp.relativeRegionAddress;
            const uintptr_t end     = mrp.relativeEnd();

            while (ptrHead < end)
            {
                const uintptr_t actualChunkSize =
                    (end - ptrHead > chunkSize) ? chunkSize :
                                                  end - ptrHead;
                res.push_back(mrp);
                res.back().relativeRegionSize    = actualChunkSize;
                res.back().relativeRegionAddress = ptrHead;
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

    template <class Self>
    Self Map::VecOp::exactName(this const Self&       self,
                               const std::string_view string)
    {
        Self rl;
        for (size_t i = 0; i < self.size(); i++)
        {
            if (*(self.at(i).regionName_sp) == string)
            {
                rl.push_back(self.at(i));
            }
        }

        return rl;
    }
    template <class Self>
    Self Map::VecOp::subName(this const Self&       self,
                             const std::string_view string)
    {
        Self rl;
        for (size_t i = 0; i < self.size(); i++)
        {
            if (self.at(i).regionName_sp->contains(string))
            {
                rl.push_back(self.at(i));
            }
        }

        return rl;
    }

    template <class Self>
    Self Map::VecOp::exactPerms(this const Self&       self,
                                const std::string_view perms)
    {
        Perms permsToMatch = Detail::parsePerms(perms);
        Self  rl;
        for (size_t i = 0; i < self.size(); i++)
        {
            const Perms p = self.at(i).perms;
            if (p == permsToMatch)
            {
                rl.push_back(self.at(i));
            }
        }
        return rl;
    }
    template <class Self>
    Self Map::VecOp::hasPerms(this const Self&       self,
                              const std::string_view perms)
    {
        Perms permsToMatch = Detail::parsePerms(perms);
        Self  rl;
        for (size_t i = 0; i < self.size(); i++)
        {
            const Perms p = self.at(i).perms;
            if ((p & permsToMatch) == permsToMatch)
            {
                rl.push_back(self.at(i));
            }
        }
        return rl;
    }
    template <class Self>
    Self Map::VecOp::notPerms(this const Self&       self,
                              const std::string_view perms)
    {
        Perms permsToMatch = Detail::parsePerms(perms);
        Self  rl;
        for (size_t i = 0; i < self.size(); i++)
        {
            const Perms p = self.at(i).perms;
            if ((p & permsToMatch) != permsToMatch)
            {
                rl.push_back(self.at(i));
            }
        }
        return rl;
    }

    template <class Self>
    Self Map::VecOp::active(this const Self& self, pid_t pid)
    {
        using namespace Utils::Literals;
        if (self.size() == 0)
        {
            rmf_Warning(
                "Given an empty types::MemoryRegionPropertiesVec!!!");
            return {};
        }
        Self              regions;
        long              pageSize    = sysconf(_SC_PAGE_SIZE);
        const std::string pagemapPath = "/proc/{}/pagemap"_f.fmt(pid);
        int               fd = open(pagemapPath.c_str(), O_RDONLY);
        if (fd < 0)
        {
            rmf_Error("Failed to open the pageMap!!!!");
            return {};
        }
        static constexpr uint64_t ACTIVE_BIT = (1ULL << 63);
        for (const auto& mrp : self)
        {
            for (uintptr_t addr = mrp.tbegin();
                 addr < mrp.tbegin() + mrp.relativeSize;
                 addr += pageSize)
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
                if (read(fd, &entry, 8) != 8)
                {
                    rmf_Error("Failed to READ the pagemap!");
                    perror("failed to read pagemap");
                    continue;
                }

                if (entry & ACTIVE_BIT)
                {
                    if (regions.size() > 0 &&
                        regions.back().rend() ==
                            addr - mrp.parentAddress)
                    {
                        regions.back().relativeSize += pageSize;
                    }
                    else
                    {
                        typename Self::InnerType newMrp = mrp;
                        newMrp.relativeSize             = pageSize;
                        newMrp.relativeAddress =
                            addr - mrp.parentAddress;
                        regions.push_back(newMrp);
                    }
                }
            }
        }
        close(fd);
        return regions;
    }
}
#endif // map_hpp_INCLUDED
