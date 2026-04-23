#ifndef map_hpp_INCLUDED
#define map_hpp_INCLUDED
#include <cstddef>
#include <magic_enum/magic_enum.hpp>
#include <cstdint>
#include <memory>
#include <type_traits>
#include "rmf/utils/str.hpp"

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
    template <typename Base>
    struct Map
    {
        static std::shared_ptr<const std::string> defaultName;
        // Default values for safety
        uintptr_t                          parentAddress   = 0;
        uintptr_t                          parentSize      = 0;
        ptrdiff_t                          relativeAddress = 0;
        ptrdiff_t                          relativeSize    = 0;
        std::shared_ptr<const std::string> regionName_sp =
            defaultName;

        using usesMap = std::true_type;

        Perms perms = Perms::None;
        // Returns the address of the beginning of this region.
        constexpr uintptr_t tbegin() const;
        // Returns the address of the end of this region (exclusive).
        constexpr uintptr_t tend() const;
        // Returns the relative beginning (relative to the parent)
        constexpr uintptr_t rbegin() const;
        // Returns the relative beginning (relative to the parent)
        constexpr uintptr_t rend() const;
        // Returns the parent beginning
        constexpr uintptr_t pbegin() const;
        // Returns the parent beginning
        constexpr uintptr_t pend() const;
                            operator std::string() const;
        bool operator==(const Map& other) const = default;
    };
}

namespace RealtimeMemoryForensics
{
    namespace Detail
    {
        inline std::shared_ptr<const std::string> defaultName =
            std::make_shared<const std::string>("");
    }

    template <typename Base>
    std::shared_ptr<const std::string> Map<Base>::defaultName =
        Detail::defaultName;

    // Returns the address of the beginning of this region.
    template <typename Base>
    constexpr uintptr_t Map<Base>::tbegin() const
    { return parentAddress + relativeAddress; }
    // Returns the address of the end of this region (exclusive).
    template <typename Base>
    constexpr uintptr_t Map<Base>::tend() const
    { return parentAddress + relativeAddress + relativeSize; }
    // Returns the relative beginning (relative to the parent)
    template <typename Base>
    constexpr uintptr_t Map<Base>::rbegin() const
    { return relativeAddress; }
    // Returns the relative beginning (relative to the parent)
    template <typename Base>
    constexpr uintptr_t Map<Base>::rend() const
    { return relativeAddress + relativeSize; }
    // Debugging use?
    template <typename Base>
    Map<Base>::operator std::string() const
    {
        using namespace RealtimeMemoryForensics::Utils::Literals;
        return "[{}] - Parent Region: [{}, {}) Actual Region: [{}, {})"_f
            .fmt(*regionName_sp, pbegin(), pend(), tbegin(), tend());
    }
    // Returns the parent beginning
    template <typename Base>
    constexpr uintptr_t Map<Base>::pbegin() const
    { return parentAddress; }
    // Returns the parent beginning
    template <typename Base>
    constexpr uintptr_t Map<Base>::pend() const
    { return parentAddress + parentSize; }
}
#endif // map_hpp_INCLUDED
