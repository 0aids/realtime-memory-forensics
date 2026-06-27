#pragma once
#include "rmf/config.hpp"
#include "rmf/maps.hpp"
#include "rmf/snapshots.hpp"
#include <cassert>
#include <concepts>
#include <ranges>

namespace rmf
{
    template <template <typename> typename SnapshotVectorLike =
                  config::DefaultVectorLike>
    struct OpInput
    {
        const Map&                          map;
        const Snapshot<SnapshotVectorLike>& snap;
    };

    template <template <typename> typename SnapshotVectorLike =
                  config::DefaultVectorLike>
    struct OpOutput
    {
        Map                          map;
        Snapshot<SnapshotVectorLike> snap;
    };

    template <template <typename> typename OpOutputContainerLike,
              typename OpOutput_t>
    concept OpOutputContainerCpt =
        requires { std::ranges::range<OpOutputContainerLike<OpOutput_t>>; };

    template <template <typename> typename SnapshotVectorLikeIn1 =
                  config::DefaultVectorLike,
              template <typename> typename SnapshotVectorLikeIn2 =
                  config::DefaultVectorLike,
              template <typename> typename SnapshotVectorLikeOut =
                  config::DefaultVectorLike,
              template <typename> typename OpOutputContainerLike =
                  config::DefaultVectorLike>
    OpOutputContainerLike<OpOutput<SnapshotVectorLikeOut>>
    findChanged(OpInput<SnapshotVectorLikeIn1> in1,
                OpInput<SnapshotVectorLikeIn1> in2, uintptr_t compareSize)
        requires SnapshotVectorCpt<SnapshotVectorLikeIn1> &&
                 SnapshotVectorCpt<SnapshotVectorLikeIn2> &&
                 SnapshotVectorCpt<SnapshotVectorLikeOut> &&
                 OpOutputContainerCpt<OpOutputContainerLike,
                                      OpOutput<SnapshotVectorLikeOut>>;

    template <template <typename> typename SnapshotVectorLikeIn1 =
                  config::DefaultVectorLike,
              template <typename> typename SnapshotVectorLikeIn2 =
                  config::DefaultVectorLike,
              template <typename> typename SnapshotVectorLikeOut =
                  config::DefaultVectorLike,
              template <typename> typename OpOutputContainerLike =
                  config::DefaultVectorLike>
    OpOutputContainerLike<OpOutput<SnapshotVectorLikeOut>>
    findUnchanged(OpInput<SnapshotVectorLikeIn1> in1,
                  OpInput<SnapshotVectorLikeIn1> in2, uintptr_t compareSize)
        requires SnapshotVectorCpt<SnapshotVectorLikeIn1> &&
                 SnapshotVectorCpt<SnapshotVectorLikeIn2> &&
                 SnapshotVectorCpt<SnapshotVectorLikeOut> &&
                 OpOutputContainerCpt<OpOutputContainerLike,
                                      OpOutput<SnapshotVectorLikeOut>>;

    template <Numeric N,
              template <typename> typename SnapshotVectorLikeIn1 =
                  config::DefaultVectorLike,
              template <typename> typename SnapshotVectorLikeIn2 =
                  config::DefaultVectorLike,
              template <typename> typename SnapshotVectorLikeOut =
                  config::DefaultVectorLike,
              template <typename> typename OpOutputContainerLike =
                  config::DefaultVectorLike>
    OpOutputContainerLike<OpOutput<SnapshotVectorLikeOut>>
    findNumChanged(OpInput<SnapshotVectorLikeIn1> in1,
                   OpInput<SnapshotVectorLikeIn1> in2, N minChangeRequired)
        requires SnapshotVectorCpt<SnapshotVectorLikeIn1> &&
                 SnapshotVectorCpt<SnapshotVectorLikeIn2> &&
                 SnapshotVectorCpt<SnapshotVectorLikeOut> &&
                 OpOutputContainerCpt<OpOutputContainerLike,
                                      OpOutput<SnapshotVectorLikeOut>>;

    template <Numeric N,
              template <typename> typename SnapshotVectorLikeIn1 =
                  config::DefaultVectorLike,
              template <typename> typename SnapshotVectorLikeIn2 =
                  config::DefaultVectorLike,
              template <typename> typename SnapshotVectorLikeOut =
                  config::DefaultVectorLike,
              template <typename> typename OpOutputContainerLike =
                  config::DefaultVectorLike>
    OpOutputContainerLike<OpOutput<SnapshotVectorLikeOut>>
    findNumUnchanged(OpInput<SnapshotVectorLikeIn1> in1,
                     OpInput<SnapshotVectorLikeIn1> in2, N maxChangeRequired)
        requires SnapshotVectorCpt<SnapshotVectorLikeIn1> &&
                 SnapshotVectorCpt<SnapshotVectorLikeIn2> &&
                 SnapshotVectorCpt<SnapshotVectorLikeOut> &&
                 OpOutputContainerCpt<OpOutputContainerLike,
                                      OpOutput<SnapshotVectorLikeOut>>;

    template <template <typename> typename SnapshotVectorLikeIn1 =
                  config::DefaultVectorLike,
              template <typename> typename SnapshotVectorLikeOut =
                  config::DefaultVectorLike,
              template <typename> typename OpOutputContainerLike =
                  config::DefaultVectorLike>
    OpOutputContainerLike<OpOutput<SnapshotVectorLikeOut>>
    findString(OpInput<SnapshotVectorLikeIn1> in1, const std::string_view str)
        requires SnapshotVectorCpt<SnapshotVectorLikeIn1> &&
                 SnapshotVectorCpt<SnapshotVectorLikeOut> &&
                 OpOutputContainerCpt<OpOutputContainerLike,
                                      OpOutput<SnapshotVectorLikeOut>>;

    template <Numeric N,
              template <typename> typename SnapshotVectorLikeIn1 =
                  config::DefaultVectorLike,
              template <typename> typename SnapshotVectorLikeOut =
                  config::DefaultVectorLike,
              template <typename> typename OpOutputContainerLike =
                  config::DefaultVectorLike>
    OpOutputContainerLike<OpOutput<SnapshotVectorLikeOut>>
    findNumExact(OpInput<SnapshotVectorLikeIn1> in1, N num)
        requires SnapshotVectorCpt<SnapshotVectorLikeIn1> &&
                 SnapshotVectorCpt<SnapshotVectorLikeOut> &&
                 OpOutputContainerCpt<OpOutputContainerLike,
                                      OpOutput<SnapshotVectorLikeOut>>;

    template <Numeric N,
              template <typename> typename SnapshotVectorLikeIn1 =
                  config::DefaultVectorLike,
              template <typename> typename SnapshotVectorLikeOut =
                  config::DefaultVectorLike,
              template <typename> typename OpOutputContainerLike =
                  config::DefaultVectorLike>
    OpOutputContainerLike<OpOutput<SnapshotVectorLikeOut>>
    findNumInRange(OpInput<SnapshotVectorLikeIn1> in1, N min, N max)
        requires SnapshotVectorCpt<SnapshotVectorLikeIn1> &&
                 SnapshotVectorCpt<SnapshotVectorLikeOut> &&
                 OpOutputContainerCpt<OpOutputContainerLike,
                                      OpOutput<SnapshotVectorLikeOut>>;
} // namespace rmf

namespace rmf
{
    template <template <typename> typename SnapshotVectorLikeIn1,
              template <typename> typename SnapshotVectorLikeIn2,
              template <typename> typename SnapshotVectorLikeOut,
              template <typename> typename OpOutputContainerLike>
    OpOutputContainerLike<OpOutput<SnapshotVectorLikeOut>>
    findChanged(OpInput<SnapshotVectorLikeIn1> in1,
                OpInput<SnapshotVectorLikeIn1> in2, uintptr_t compareSize)
        requires SnapshotVectorCpt<SnapshotVectorLikeIn1> &&
                 SnapshotVectorCpt<SnapshotVectorLikeIn2> &&
                 SnapshotVectorCpt<SnapshotVectorLikeOut> &&
                 OpOutputContainerCpt<OpOutputContainerLike,
                                      OpOutput<SnapshotVectorLikeOut>>
    {
        assert(false && "TODO!");
    }

    template <template <typename> typename SnapshotVectorLikeIn1,
              template <typename> typename SnapshotVectorLikeIn2,
              template <typename> typename SnapshotVectorLikeOut,
              template <typename> typename OpOutputContainerLike>
    OpOutputContainerLike<OpOutput<SnapshotVectorLikeOut>>
    findUnchanged(OpInput<SnapshotVectorLikeIn1> in1,
                  OpInput<SnapshotVectorLikeIn1> in2, uintptr_t compareSize)
        requires SnapshotVectorCpt<SnapshotVectorLikeIn1> &&
                 SnapshotVectorCpt<SnapshotVectorLikeIn2> &&
                 SnapshotVectorCpt<SnapshotVectorLikeOut> &&
                 OpOutputContainerCpt<OpOutputContainerLike,
                                      OpOutput<SnapshotVectorLikeOut>>
    {
        assert(false && "TODO!");
    }

    template <Numeric N, template <typename> typename SnapshotVectorLikeIn1,
              template <typename> typename SnapshotVectorLikeIn2,
              template <typename> typename SnapshotVectorLikeOut,
              template <typename> typename OpOutputContainerLike>
    OpOutputContainerLike<OpOutput<SnapshotVectorLikeOut>>
    findNumChanged(OpInput<SnapshotVectorLikeIn1> in1,
                   OpInput<SnapshotVectorLikeIn1> in2, N minChangeRequired)
        requires SnapshotVectorCpt<SnapshotVectorLikeIn1> &&
                 SnapshotVectorCpt<SnapshotVectorLikeIn2> &&
                 SnapshotVectorCpt<SnapshotVectorLikeOut> &&
                 OpOutputContainerCpt<OpOutputContainerLike,
                                      OpOutput<SnapshotVectorLikeOut>>
    {
        assert(false && "TODO!");
    }

    template <Numeric N, template <typename> typename SnapshotVectorLikeIn1,
              template <typename> typename SnapshotVectorLikeIn2,
              template <typename> typename SnapshotVectorLikeOut,
              template <typename> typename OpOutputContainerLike>
    OpOutputContainerLike<OpOutput<SnapshotVectorLikeOut>>
    findNumUnchanged(OpInput<SnapshotVectorLikeIn1> in1,
                     OpInput<SnapshotVectorLikeIn1> in2, N maxChangeRequired)
        requires SnapshotVectorCpt<SnapshotVectorLikeIn1> &&
                 SnapshotVectorCpt<SnapshotVectorLikeIn2> &&
                 SnapshotVectorCpt<SnapshotVectorLikeOut> &&
                 OpOutputContainerCpt<OpOutputContainerLike,
                                      OpOutput<SnapshotVectorLikeOut>>
    {
        assert(false && "TODO!");
    }

    template <template <typename> typename SnapshotVectorLikeIn1,
              template <typename> typename SnapshotVectorLikeOut,
              template <typename> typename OpOutputContainerLike>
    OpOutputContainerLike<OpOutput<SnapshotVectorLikeOut>>
    findString(OpInput<SnapshotVectorLikeIn1> in1, const std::string_view str)
        requires SnapshotVectorCpt<SnapshotVectorLikeIn1> &&
                 SnapshotVectorCpt<SnapshotVectorLikeOut> &&
                 OpOutputContainerCpt<OpOutputContainerLike,
                                      OpOutput<SnapshotVectorLikeOut>>
    {
        assert(false && "TODO!");
    }

    template <Numeric N, template <typename> typename SnapshotVectorLikeIn1,
              template <typename> typename SnapshotVectorLikeOut,
              template <typename> typename OpOutputContainerLike>
    OpOutputContainerLike<OpOutput<SnapshotVectorLikeOut>>
    findNumExact(OpInput<SnapshotVectorLikeIn1> in1, N num)
        requires SnapshotVectorCpt<SnapshotVectorLikeIn1> &&
                 SnapshotVectorCpt<SnapshotVectorLikeOut> &&
                 OpOutputContainerCpt<OpOutputContainerLike,
                                      OpOutput<SnapshotVectorLikeOut>>
    {
        assert(false && "TODO!");
    }

    template <Numeric N, template <typename> typename SnapshotVectorLikeIn1,
              template <typename> typename SnapshotVectorLikeOut,
              template <typename> typename OpOutputContainerLike>
    OpOutputContainerLike<OpOutput<SnapshotVectorLikeOut>>
    findNumInRange(OpInput<SnapshotVectorLikeIn1> in1, N min, N max)
        requires SnapshotVectorCpt<SnapshotVectorLikeIn1> &&
                 SnapshotVectorCpt<SnapshotVectorLikeOut> &&
                 OpOutputContainerCpt<OpOutputContainerLike,
                                      OpOutput<SnapshotVectorLikeOut>>
    {
        assert(false && "TODO!");
    }
}
