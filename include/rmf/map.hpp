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
            bool operator==(const MapData& other) const = default;
        };
    }
    template <typename Base>
    struct Map
    {
        Detail::MapData map;
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

        // oh my gah
        template <template <typename> typename... Features>
        struct Modifier
        {
            using Result_t = Node<Features...>;
            std::function<Result_t()> func;
            Result_t                  operator()();
        };
        template <typename BaseVec>
        struct VecOp
        {
            // using InnerType = BaseVec::InnerType;
            BaseVec& minSize(size_t) const;
            BaseVec& maxSize(size_t) const;
            BaseVec& chunkify(size_t) const;

            // TODO: Move naming filters to regex.
            BaseVec& exactName(const std::string_view) const;
            BaseVec& subName(const std::string_view) const;

            BaseVec& exactPerms(Perms) const;
            BaseVec& hasPerms(Perms) const;
            BaseVec& notPerms(Perms) const;
            BaseVec& active(pid_t pid) const;
        };
    };
}

namespace RealtimeMemoryForensics
{
    // Returns the address of the beginning of this region.
    template <typename Base>
    constexpr uintptr_t Map<Base>::tbegin() const
    { return map.parentAddress + map.relativeAddress; }
    // Returns the address of the end of this region (exclusive).
    template <typename Base>
    constexpr uintptr_t Map<Base>::tend() const
    {
        return map.parentAddress + map.relativeAddress +
            map.relativeSize;
    }
    // Returns the relative beginning (relative to the parent)
    template <typename Base>
    constexpr uintptr_t Map<Base>::rbegin() const
    { return map.relativeAddress; }
    // Returns the relative beginning (relative to the parent)
    template <typename Base>
    constexpr uintptr_t Map<Base>::rend() const
    { return map.relativeAddress + map.relativeSize; }
    // Debugging use?
    template <typename Base>
    Map<Base>::operator std::string() const
    {
        using namespace RealtimeMemoryForensics::Utils::Literals;
        return "[{}] - Parent Region: [{}, {}) Actual Region: [{}, {})"_f
            .fmt(*map.regionName_sp, pbegin(), pend(), tbegin(),
                 tend());
    }
    // Returns the parent beginning
    template <typename Base>
    constexpr uintptr_t Map<Base>::pbegin() const
    { return map.parentAddress; }
    // Returns the parent beginning
    template <typename Base>
    constexpr uintptr_t Map<Base>::pend() const
    { return map.parentAddress + map.parentSize; }
}
#endif // map_hpp_INCLUDED
