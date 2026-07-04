#pragma once

#include <cstdint>
#include <memory>
#include <vector>

namespace rmf
{
    // A snapshot should normally be paired with a map.
    // Snapshots generated from maps start from tbegin to tend in the virtual address space.
    // Their size is tsize.
    using Snapshot = std::vector<uint8_t>;
} // namespace rmf
