#include "memory_region.hpp"
#include "threadpoolv2.hpp"
#include "region_pool.hpp"
#include "analysis_threadpool.hpp"
#include "log.hpp"
#include "region_properties.hpp"
#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstring>
extern "C"
{
#include <bits/types/struct_iovec.h>
#include <sys/uio.h>
}

static inline auto SmallRegionDiscriminator =
    [](ssize_t i, std::vector<MemoryRegionProperties>&rl) -> SmallRegionCapture {
        return {};
    };
static constexpr auto SmallRegionConsolidator = []() {};

void MemoryRegionPool::threadSmallerRegions(RegionPoolCallback rpcb)
{
    std::vector<MemoryRegionProperties*> smallRegionProperties;
    smallRegionProperties.reserve(
        m_regionResultsHistory_l.back().size());

    rpcb;
    uintptr_t totalSize = 0;

    for (auto& smallRegion : m_regionResultsHistory_l.back())
    {
        if (smallRegion.regionSize <
            RegionAnalysisThreadPool::smallRegionThreshold)
        {
            smallRegionProperties.push_back(&smallRegion);
            totalSize += smallRegion.regionSize;
        }
    }

    uintptr_t bytesPerThread =
        totalSize / RegionAnalysisThreadPool::numThreads;

    // Each thread will:
    // Get a list of memory regions that it needs to complete
    // Collate these results into an internal buffer.
    ThreadPool<MemoryRegionProperties, SmallRegionCapture> tp(
        // Some god awful shit in here.
        SmallRegionDiscriminator,
        rpcb,
        niceNumThreads
    );


    // Afterwards we will collate all those results into another final vector.
}

void MemoryRegionPool::findChangedRegions(size_t numBytes) {}
