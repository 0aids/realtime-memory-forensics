#pragma once
#include <vector>
#include <thread>
#include <functional>
#include "region_properties.hpp"
#include <cstdlib>
inline size_t NUMTHREADS = std::thread::hardware_concurrency() - 2;

struct RegionAnalysisCapture
{
    // Relative to the subregion, not to the actual.
    uintptr_t             start; // Start is relative and inclusive.
    uintptr_t             size;  // ( start + size ) is not inclusive.
    RegionPropertiesList& results;
};

using RegionAnalysisFunction =
    std::function<void(RegionAnalysisCapture)>;

class RegionAnalysisThread : public std::thread
{
  private:
    RegionAnalysisCapture m_capture;

  public:
    RegionAnalysisThread(RegionAnalysisCapture         capture,
                         const RegionAnalysisFunction& func) :
        std::thread(func, capture), m_capture(capture)
    {
    }
};

// A very basic threadpool that makes use of lambdas and automatic
// sub-region allocation, with support for overlaps.
class RegionAnalysisThreadPool
{
  private:
    size_t                            m_numThreads;
    size_t                            m_overlap;
    std::vector<RegionAnalysisThread> m_threadPool;
    size_t                            m_totalResults;
    const MemoryRegionProperties&     m_regionProperties;
    std::vector<RegionPropertiesList> m_preliminaryResultsPool;

	// Supposedly a flag to show if the threads have been completed
	// prevents stupid shit from happening, but idrc
	// It's not like anyone else will use this program.
    bool                              m_complete = false;

  public:

    // Number of threads to leave free when multithreading.
    inline static size_t numFreeThreads = 2;
    inline static size_t maxNumThreads = std::thread::hardware_concurrency();
    inline static size_t numThreads = maxNumThreads - numFreeThreads;
    inline static uintptr_t  smallRegionThreshold = 1 << 30;
    inline static std::atomic<bool> forceSingleThreadingGlobally = false;

    RegionPropertiesList m_resultsPool;
    RegionAnalysisThreadPool(
        size_t numThreads, size_t overlap,
        const MemoryRegionProperties& properties);

    void startAllThreads(const RegionAnalysisFunction& func);

    void joinAllThreads();

    RegionPropertiesList
    consolidateResults(bool extendConnected = false,
                       bool checkDuplicates = false);
};
