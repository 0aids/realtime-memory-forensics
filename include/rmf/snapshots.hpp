#pragma once

#include <cstdint>
#include <memory>
#include <ranges>
namespace rmf
{
    template <template <typename...> typename VectorLike>
    concept SnapshotVectorCpt =
        requires { std::ranges::contiguous_range<VectorLike<uint8_t>>; };
    // A snapshot should normally be paired with a map.
    // Snapshots generated from maps start from tbegin to tend in the virtual address space.
    // Their size is tsize.
    template <template <typename> typename VectorLike>
        requires SnapshotVectorCpt<VectorLike>
    struct Snapshot
    {
        using SnapshotVector                 = VectorLike<uint8_t>;
        std::shared_ptr<SnapshotVector> data = nullptr;
    };
} // namespace rmf
