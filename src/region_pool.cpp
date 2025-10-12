#include "memory_region.hpp"
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
    RegionAnalysisThreadPool tp(RegionAnalysisThreadPool::numThreads, 0, {});


    // Afterwards we will collate all those results into another final vector.
}

void MemoryRegionPool::findChangedRegions(size_t numBytes) {}
