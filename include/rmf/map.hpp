#ifndef map_hpp_INCLUDED
#define map_hpp_INCLUDED
#include <cstddef>
#include <magic_enum/magic_enum.hpp>
#include <cstdint>
#include <memory>
#include <type_traits>
#include "rmf/utils/str.hpp"
#include "rmf/node.hpp"

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
            Self& minSize(this Self& self, size_t);
            template <class Self>
            Self& maxSize(this Self& self, size_t);
            template <class Self>
            Self& chunkify(this Self& self, size_t);

            // TODO: Move naming filters to regex.

            template <class Self>
            Self& exactName(this Self& self, const std::string_view);
            template <class Self>
            Self& subName(this Self& self, const std::string_view);

            template <class Self>
            Self& exactPerms(this Self& self, Perms);
            template <class Self>
            Self& hasPerms(this Self& self, Perms);
            template <class Self>
            Self& notPerms(this Self& self, Perms);
            template <class Self>
            Self& active(this Self& self, pid_t pid);
        };
    };
}

namespace RealtimeMemoryForensics
{
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
}
#endif // map_hpp_INCLUDED
