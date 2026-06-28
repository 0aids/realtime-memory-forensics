#pragma once
#include "rmf/config.hpp"
#include "rmf/snapshots.hpp"
#include "rmf/memory_region.hpp"

#include <cassert>

namespace rmf
{

    template <template <typename> typename SnapshotVectorLikeIn1 =
                  config::DefaultVectorLike,
              template <typename> typename SnapshotVectorLikeIn2 =
                  config::DefaultVectorLike,
              template <typename> typename SnapshotVectorLikeOut =
                  config::DefaultVectorLike,
              template <typename> typename MemoryRegionContainerLike =
                  config::DefaultVectorLike>
    MemoryRegionContainerLike<MemoryRegion<SnapshotVectorLikeOut>>
    findChanged(MemoryRegionView<SnapshotVectorLikeIn1> in1,
                MemoryRegionView<SnapshotVectorLikeIn1> in2,
                uintptr_t                               compareSize)
        requires SnapshotVectorCpt<SnapshotVectorLikeIn1> &&
                 SnapshotVectorCpt<SnapshotVectorLikeIn2> &&
                 SnapshotVectorCpt<SnapshotVectorLikeOut> &&
                 MemoryRegionContainerCpt<MemoryRegionContainerLike,
                                          MemoryRegion<SnapshotVectorLikeOut>>;

    template <template <typename> typename SnapshotVectorLikeIn1 =
                  config::DefaultVectorLike,
              template <typename> typename SnapshotVectorLikeIn2 =
                  config::DefaultVectorLike,
              template <typename> typename SnapshotVectorLikeOut =
                  config::DefaultVectorLike,
              template <typename> typename MemoryRegionContainerLike =
                  config::DefaultVectorLike>
    MemoryRegionContainerLike<MemoryRegion<SnapshotVectorLikeOut>>
    findUnchanged(MemoryRegionView<SnapshotVectorLikeIn1> in1,
                  MemoryRegionView<SnapshotVectorLikeIn1> in2,
                  uintptr_t                               compareSize)
        requires SnapshotVectorCpt<SnapshotVectorLikeIn1> &&
                 SnapshotVectorCpt<SnapshotVectorLikeIn2> &&
                 SnapshotVectorCpt<SnapshotVectorLikeOut> &&
                 MemoryRegionContainerCpt<MemoryRegionContainerLike,
                                          MemoryRegion<SnapshotVectorLikeOut>>;

    template <Numeric N,
              template <typename> typename SnapshotVectorLikeIn1 =
                  config::DefaultVectorLike,
              template <typename> typename SnapshotVectorLikeIn2 =
                  config::DefaultVectorLike,
              template <typename> typename SnapshotVectorLikeOut =
                  config::DefaultVectorLike,
              template <typename> typename MemoryRegionContainerLike =
                  config::DefaultVectorLike>
    MemoryRegionContainerLike<MemoryRegion<SnapshotVectorLikeOut>>
    findNumChanged(MemoryRegionView<SnapshotVectorLikeIn1> in1,
                   MemoryRegionView<SnapshotVectorLikeIn1> in2,
                   N                                       minChangeRequired)
        requires SnapshotVectorCpt<SnapshotVectorLikeIn1> &&
                 SnapshotVectorCpt<SnapshotVectorLikeIn2> &&
                 SnapshotVectorCpt<SnapshotVectorLikeOut> &&
                 MemoryRegionContainerCpt<MemoryRegionContainerLike,
                                          MemoryRegion<SnapshotVectorLikeOut>>;

    template <Numeric N,
              template <typename> typename SnapshotVectorLikeIn1 =
                  config::DefaultVectorLike,
              template <typename> typename SnapshotVectorLikeIn2 =
                  config::DefaultVectorLike,
              template <typename> typename SnapshotVectorLikeOut =
                  config::DefaultVectorLike,
              template <typename> typename MemoryRegionContainerLike =
                  config::DefaultVectorLike>
    MemoryRegionContainerLike<MemoryRegion<SnapshotVectorLikeOut>>
    findNumUnchanged(MemoryRegionView<SnapshotVectorLikeIn1> in1,
                     MemoryRegionView<SnapshotVectorLikeIn1> in2,
                     N                                       maxChangeRequired)
        requires SnapshotVectorCpt<SnapshotVectorLikeIn1> &&
                 SnapshotVectorCpt<SnapshotVectorLikeIn2> &&
                 SnapshotVectorCpt<SnapshotVectorLikeOut> &&
                 MemoryRegionContainerCpt<MemoryRegionContainerLike,
                                          MemoryRegion<SnapshotVectorLikeOut>>;

    template <template <typename> typename SnapshotVectorLikeIn1 =
                  config::DefaultVectorLike,
              template <typename> typename SnapshotVectorLikeOut =
                  config::DefaultVectorLike,
              template <typename> typename MemoryRegionContainerLike =
                  config::DefaultVectorLike>
    MemoryRegionContainerLike<MemoryRegion<SnapshotVectorLikeOut>>
    findString(MemoryRegionView<SnapshotVectorLikeIn1> in1,
               const std::string_view                  str)
        requires SnapshotVectorCpt<SnapshotVectorLikeIn1> &&
                 SnapshotVectorCpt<SnapshotVectorLikeOut> &&
                 MemoryRegionContainerCpt<MemoryRegionContainerLike,
                                          MemoryRegion<SnapshotVectorLikeOut>>;

    template <Numeric N,
              template <typename> typename SnapshotVectorLikeIn1 =
                  config::DefaultVectorLike,
              template <typename> typename SnapshotVectorLikeOut =
                  config::DefaultVectorLike,
              template <typename> typename MemoryRegionContainerLike =
                  config::DefaultVectorLike>
    MemoryRegionContainerLike<MemoryRegion<SnapshotVectorLikeOut>>
    findNumExact(MemoryRegionView<SnapshotVectorLikeIn1> in1, N num)
        requires SnapshotVectorCpt<SnapshotVectorLikeIn1> &&
                 SnapshotVectorCpt<SnapshotVectorLikeOut> &&
                 MemoryRegionContainerCpt<MemoryRegionContainerLike,
                                          MemoryRegion<SnapshotVectorLikeOut>>;

    template <Numeric N,
              template <typename> typename SnapshotVectorLikeIn1 =
                  config::DefaultVectorLike,
              template <typename> typename SnapshotVectorLikeOut =
                  config::DefaultVectorLike,
              template <typename> typename MemoryRegionContainerLike =
                  config::DefaultVectorLike>
    MemoryRegionContainerLike<MemoryRegion<SnapshotVectorLikeOut>>
    findNumInRange(MemoryRegionView<SnapshotVectorLikeIn1> in1, N min, N max)
        requires SnapshotVectorCpt<SnapshotVectorLikeIn1> &&
                 SnapshotVectorCpt<SnapshotVectorLikeOut> &&
                 MemoryRegionContainerCpt<MemoryRegionContainerLike,
                                          MemoryRegion<SnapshotVectorLikeOut>>;
} // namespace rmf

namespace rmf
{
    template <template <typename> typename SnapshotVectorLikeIn1,
              template <typename> typename SnapshotVectorLikeIn2,
              template <typename> typename SnapshotVectorLikeOut,
              template <typename> typename MemoryRegionContainerLike>
    MemoryRegionContainerLike<MemoryRegion<SnapshotVectorLikeOut>>
    findChanged(MemoryRegionView<SnapshotVectorLikeIn1> in1,
                MemoryRegionView<SnapshotVectorLikeIn1> in2,
                uintptr_t                               compareSize)
        requires SnapshotVectorCpt<SnapshotVectorLikeIn1> &&
                 SnapshotVectorCpt<SnapshotVectorLikeIn2> &&
                 SnapshotVectorCpt<SnapshotVectorLikeOut> &&
                 MemoryRegionContainerCpt<MemoryRegionContainerLike,
                                          MemoryRegion<SnapshotVectorLikeOut>>
    {
        assert(false && "TODO!");
    }

    template <template <typename> typename SnapshotVectorLikeIn1,
              template <typename> typename SnapshotVectorLikeIn2,
              template <typename> typename SnapshotVectorLikeOut,
              template <typename> typename MemoryRegionContainerLike>
    MemoryRegionContainerLike<MemoryRegion<SnapshotVectorLikeOut>>
    findUnchanged(MemoryRegionView<SnapshotVectorLikeIn1> in1,
                  MemoryRegionView<SnapshotVectorLikeIn1> in2,
                  uintptr_t                               compareSize)
        requires SnapshotVectorCpt<SnapshotVectorLikeIn1> &&
                 SnapshotVectorCpt<SnapshotVectorLikeIn2> &&
                 SnapshotVectorCpt<SnapshotVectorLikeOut> &&
                 MemoryRegionContainerCpt<MemoryRegionContainerLike,
                                          MemoryRegion<SnapshotVectorLikeOut>>
    {
        assert(false && "TODO!");
    }

    template <Numeric N, template <typename> typename SnapshotVectorLikeIn1,
              template <typename> typename SnapshotVectorLikeIn2,
              template <typename> typename SnapshotVectorLikeOut,
              template <typename> typename MemoryRegionContainerLike>
    MemoryRegionContainerLike<MemoryRegion<SnapshotVectorLikeOut>>
    findNumChanged(MemoryRegionView<SnapshotVectorLikeIn1> in1,
                   MemoryRegionView<SnapshotVectorLikeIn1> in2,
                   N                                       minChangeRequired)
        requires SnapshotVectorCpt<SnapshotVectorLikeIn1> &&
                 SnapshotVectorCpt<SnapshotVectorLikeIn2> &&
                 SnapshotVectorCpt<SnapshotVectorLikeOut> &&
                 MemoryRegionContainerCpt<MemoryRegionContainerLike,
                                          MemoryRegion<SnapshotVectorLikeOut>>
    {
        assert(false && "TODO!");
    }

    template <Numeric N, template <typename> typename SnapshotVectorLikeIn1,
              template <typename> typename SnapshotVectorLikeIn2,
              template <typename> typename SnapshotVectorLikeOut,
              template <typename> typename MemoryRegionContainerLike>
    MemoryRegionContainerLike<MemoryRegion<SnapshotVectorLikeOut>>
    findNumUnchanged(MemoryRegionView<SnapshotVectorLikeIn1> in1,
                     MemoryRegionView<SnapshotVectorLikeIn1> in2,
                     N                                       maxChangeRequired)
        requires SnapshotVectorCpt<SnapshotVectorLikeIn1> &&
                 SnapshotVectorCpt<SnapshotVectorLikeIn2> &&
                 SnapshotVectorCpt<SnapshotVectorLikeOut> &&
                 MemoryRegionContainerCpt<MemoryRegionContainerLike,
                                          MemoryRegion<SnapshotVectorLikeOut>>
    {
        assert(false && "TODO!");
    }

    template <template <typename> typename SnapshotVectorLikeIn1,
              template <typename> typename SnapshotVectorLikeOut,
              template <typename> typename MemoryRegionContainerLike>
    MemoryRegionContainerLike<MemoryRegion<SnapshotVectorLikeOut>>
    findString(MemoryRegionView<SnapshotVectorLikeIn1> in1,
               const std::string_view                  str)
        requires SnapshotVectorCpt<SnapshotVectorLikeIn1> &&
                 SnapshotVectorCpt<SnapshotVectorLikeOut> &&
                 MemoryRegionContainerCpt<MemoryRegionContainerLike,
                                          MemoryRegion<SnapshotVectorLikeOut>>
    {
        assert(false && "TODO!");
    }

    template <Numeric N, template <typename> typename SnapshotVectorLikeIn1,
              template <typename> typename SnapshotVectorLikeOut,
              template <typename> typename MemoryRegionContainerLike>
    MemoryRegionContainerLike<MemoryRegion<SnapshotVectorLikeOut>>
    findNumExact(MemoryRegionView<SnapshotVectorLikeIn1> in1, N num)
        requires SnapshotVectorCpt<SnapshotVectorLikeIn1> &&
                 SnapshotVectorCpt<SnapshotVectorLikeOut> &&
                 MemoryRegionContainerCpt<MemoryRegionContainerLike,
                                          MemoryRegion<SnapshotVectorLikeOut>>
    {
        assert(false && "TODO!");
    }

    template <Numeric N, template <typename> typename SnapshotVectorLikeIn1,
              template <typename> typename SnapshotVectorLikeOut,
              template <typename> typename MemoryRegionContainerLike>
    MemoryRegionContainerLike<MemoryRegion<SnapshotVectorLikeOut>>
    findNumInRange(MemoryRegionView<SnapshotVectorLikeIn1> in1, N min, N max)
        requires SnapshotVectorCpt<SnapshotVectorLikeIn1> &&
                 SnapshotVectorCpt<SnapshotVectorLikeOut> &&
                 MemoryRegionContainerCpt<MemoryRegionContainerLike,
                                          MemoryRegion<SnapshotVectorLikeOut>>
    {
        assert(false && "TODO!");
    }
}
