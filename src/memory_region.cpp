#include "rmf/memory_region.hpp"
namespace rmf
{
    MemoryRegion::operator MemoryRegionView() const
    {
        return MemoryRegionView{
            .map  = map,
            .snap = snap,
        };
    }
} // namespace rmf
