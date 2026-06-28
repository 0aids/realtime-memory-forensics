#pragma once
#include "rmf/config.hpp"
#include "rmf/snapshots.hpp"
#include "rmf/maps.hpp"

namespace rmf
{
    struct MemoryRegionView
    {
        const Map&      map;
        const Snapshot& snap;
    };

    struct MemoryRegion
    {
        Map      map;
        Snapshot snap;

                 operator MemoryRegionView() const;
    };

    template <TypedCpt TypedMixin>
    struct MemoryRegionTyped : public TypedMixin, public MemoryRegion
    {
    };
}
