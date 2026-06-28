#pragma once
#include "rmf/config.hpp"
#include "rmf/snapshots.hpp"
#include "rmf/maps.hpp"
#include <type_traits>

namespace rmf
{
    template <template <typename> typename MemoryRegionContainerLike,
              typename MemoryRegion_t>
    concept MemoryRegionContainerCpt = requires {
        std::ranges::range<MemoryRegionContainerLike<MemoryRegion_t>>;
    };

    template <template <typename> typename SnapshotVectorLike =
                  config::DefaultVectorLike>
        requires SnapshotVectorCpt<SnapshotVectorLike>
    struct MemoryRegionView
    {
        const Map&                          map;
        const Snapshot<SnapshotVectorLike>& snap;
    };

    template <template <typename> typename SnapshotVectorLike =
                  config::DefaultVectorLike>
        requires SnapshotVectorCpt<SnapshotVectorLike>
    struct MemoryRegion
    {
        Map                          map;
        Snapshot<SnapshotVectorLike> snap;

        operator MemoryRegionView<SnapshotVectorLike>() const;
    };

    // A mixin. TypeMixin will hold the extra data for adding types and methods.
    template <typename TypeMixin,
              template <typename> typename SnapshotVectorLike =
                  config::DefaultVectorLike>
        requires SnapshotVectorCpt<SnapshotVectorLike> && TypedCpt<TypeMixin>
    struct MemoryRegionTyped : public TypeMixin,
                               public MemoryRegion<SnapshotVectorLike>
    {
    };

    template <typename T,
              template <typename, template <typename> typename> typename TT>
    static constexpr bool isTemplatedFrom_Typename_Template = std::false_type{};

    template <template <typename, template <typename> typename> typename TT,
              typename... T>
    static constexpr bool isTemplatedFrom_Typename_Template<TT<T...>, TT> =
        std::true_type{};

    template <typename T, template <template <typename> typename> typename TT>
    static constexpr bool isTemplatedFrom_Template = std::false_type{};

    template <template <template <typename> typename> typename TT,
              template <typename> typename T>
    static constexpr bool isTemplatedFrom_Template<TT<T>, TT> =
        std::true_type{};

    template <typename T>
    concept isMemoryRegionTypedCpt =
        requires { isTemplatedFrom_Typename_Template<T, MemoryRegionTyped>; };

    template <typename T>
    concept isMemoryRegionCpt =
        requires { isTemplatedFrom_Template<T, MemoryRegion>; };

    template <typename T>
    concept isMemoryRegionViewCpt =
        requires { isTemplatedFrom_Template<T, MemoryRegionView>; };
}
