#pragma once

#include <cstdint>
#include <memory>
#include <vector>

namespace rmf
{
    // A snapshot should normally be paired with a map.
    // Snapshots generated from maps start from tbegin to tend in the virtual address space.
    // Their size is tsize.
    struct Snapshot
    {
        using SnapshotVector                 = std::vector<uint8_t>;
        std::shared_ptr<SnapshotVector> data = nullptr;
    };
} // namespace rmf
