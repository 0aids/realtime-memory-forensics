#include "data/snapshots.hpp"

using namespace rmf::data;
MemorySnapshotSpan MemorySnapshot::asSnapshotSpan() const noexcept {
    return MemorySnapshotSpan(*this);
}
