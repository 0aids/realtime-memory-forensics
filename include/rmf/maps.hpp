#pragma once

#include <cstddef>
#include <cstdint>
#include <magic_enum/magic_enum.hpp>
#include <memory>
#include <string>
#include <ranges>

namespace rmf
{
    enum class Perms : uint8_t
    {
        None    = 0,
        Read    = 1 << 0,
        Write   = 1 << 2,
        Execute = 1 << 3,
        Shared  = 1 << 4,
    };
    template <std::ranges::range Range>
        requires std::same_as<std::ranges::range_value_t<Range>, char>
    Perms Perms_Parse(Range chars);
} // namespace rmf

template <>
struct magic_enum::customize::enum_range<rmf::Perms>
{
    static constexpr bool is_flags = true;
};

rmf::Perms  operator|(rmf::Perms p1, rmf::Perms p2);
rmf::Perms& operator|=(rmf::Perms& p1, rmf::Perms p2);
rmf::Perms  operator&(rmf::Perms p1, rmf::Perms p2);
rmf::Perms& operator&=(rmf::Perms& p1, rmf::Perms p2);
rmf::Perms  operator^(rmf::Perms p1, rmf::Perms p2);
rmf::Perms& operator^=(rmf::Perms& p1, rmf::Perms p2);
rmf::Perms  operator~(rmf::Perms p1);

namespace rmf
{
    struct Map
    {
        std::shared_ptr<const std::string> name  = nullptr;
        uintptr_t                          pAddr = 0;
        uintptr_t                          pSize = 0;
        ptrdiff_t                          rAddr = 0;
        ptrdiff_t                          rSize = 0;
        Perms                              perms = Perms::None;
        bool operator==(const Map& other) const  = default;

        // True address is with respect to our virtual memory space
        // This includes the relative of this region.
        constexpr uintptr_t tbegin() const;
        // Exclusive - This is one off the end.
        constexpr uintptr_t tend() const;

        // relative to the parent
        constexpr uintptr_t rbegin() const;
        // Exclusive - This is one off the end.
        constexpr uintptr_t rend() const;

        // just for the parent
        constexpr uintptr_t pbegin() const;
        // Exclusive - This is one off the end.
        constexpr uintptr_t pend() const;

        constexpr bool      valid() const;

        bool                isLTSize(ptrdiff_t size) const;
        bool                isGTSize(ptrdiff_t size) const;
        bool                isEQSize(ptrdiff_t size) const;
        bool                isExactName(const std::string_view string) const;
        bool                isSubName(const std::string_view string) const;
        bool                isExactPerms(const std::string_view perms) const;
        bool                isHavePerms(const std::string_view perms) const;
        bool                isNotHavePerms(const std::string_view perms) const;

        std::vector<Map> chunkify(ptrdiff_t size, ptrdiff_t overlap = 0) const;
    };
} // namespace rmf

// Definitions
namespace rmf
{
    constexpr uintptr_t Map::pbegin() const
    {
        return pAddr;
    }
    constexpr uintptr_t Map::rbegin() const
    {
        return rAddr;
    }
    constexpr uintptr_t Map::tbegin() const
    {
        return pAddr + rAddr;
    }
    constexpr uintptr_t Map::pend() const
    {
        return pAddr + pSize;
    }
    constexpr uintptr_t Map::rend() const
    {
        return rAddr + rSize;
    }
    constexpr uintptr_t Map::tend() const
    {
        return tbegin() + rSize;
    }

    constexpr bool Map::valid() const
    {
        return name != nullptr;
    }

    template <std::ranges::range Range>
        requires std::same_as<std::ranges::range_value_t<Range>, char>
    Perms Perms_Parse(Range chars)
    {
        Perms p = Perms::None;
        for (auto c : chars)
        {
            switch (c)
            {
                case 'r':
                case 'R':
                    p |= Perms::Read;
                    break;
                case 'w':
                case 'W':
                    p |= Perms::Write;
                    break;
                case 's':
                case 'S':
                    p |= Perms::Shared;
                    break;
                case 'x':
                case 'X':
                    p |= Perms::Execute;
                    break;
            }
        }
        return p;
    }
} // namespace rmf
