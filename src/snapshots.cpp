#include "snapshots.hpp"

MemorySnapshotSpan MemorySnapshot::asSnapshotSpan() const noexcept {
    return MemorySnapshotSpan(*this);
}
