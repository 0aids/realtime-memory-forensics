#include "analysis_threadpool.hpp"
#include "log.hpp"


RegionAnalysisThreadPool::RegionAnalysisThreadPool(
    size_t numThreads, size_t overlap,
    const MemoryRegionProperties& properties) :
    m_numThreads(numThreads), m_overlap(overlap),
    m_regionProperties(properties),
    m_preliminaryResultsPool(numThreads)
{
    m_threadPool.reserve(numThreads);
    m_totalResults = 0;
}

// WARNING: For the lambda, DO NOT USE A REFERENCE. Will need to fix by compositing
// the thread later.
// TODO: ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

void RegionAnalysisThreadPool::startAllThreads(
    const RegionAnalysisFunction& func)
{
    // Align to 64 bits. Allows for proper alignment for certain Data structures.
    uintptr_t bytesPerThread =
        ((m_regionProperties.regionSize / m_numThreads) / 8) * 8;

    Log(Debug, "Bytes per thread: " << bytesPerThread);
    Log(Debug, "Overlap: " << m_overlap);
    if (forceSingleThreadingGlobally.load()) {
        m_numThreads = 1;
    }
    for (size_t i = 1; i < m_numThreads; i++)
    {
        RegionAnalysisCapture capture = {
            // Relative to the sub region.
            .start   = i * bytesPerThread,
            .size    = bytesPerThread + m_overlap,
            .results = m_preliminaryResultsPool[i],
        };

        if (i == m_numThreads - 1)
        {
            capture.size =
                m_regionProperties.regionSize - capture.start;
        }
        Log(Debug, "Thread: " << i << " has started.");
        Log(Debug,
            "relative Start: " << std::hex << std::showbase
                               << capture.start);
        Log(Debug,
            "size: " << std::hex << std::showbase << capture.size);
        Log(Debug,
            "relative end: " << std::hex << std::showbase
                             << capture.start + capture.size);
        Log(Debug,
            "Actual Start: " << std::hex << std::showbase
                             << capture.start +
                    m_regionProperties.getActualRegionStart());
        Log(Debug,
            "Actual end: " << std::hex << std::showbase
                           << capture.start + capture.size +
                    m_regionProperties.getActualRegionStart());

        m_threadPool.push_back(RegionAnalysisThread(capture, func));
    }
    Log(Debug, "Host thread is starting...");
    RegionAnalysisCapture capture = {
        .start   = 0,
        .size    = bytesPerThread,
        .results = m_preliminaryResultsPool[0],
    };

    func(capture);
}

void RegionAnalysisThreadPool::joinAllThreads()
{
    m_totalResults = 0;
    for (size_t i = 0; i < m_numThreads; i++)
    {
        if (m_threadPool.size() > i)
            m_threadPool[i].join();
        m_totalResults += m_preliminaryResultsPool[i].size();
    }
}

RegionPropertiesList RegionAnalysisThreadPool::consolidateResults(
    bool extendConnected, bool checkDuplicates)
{
    Log(Debug,
        "Total number of changes before consolidation: "
            << m_totalResults);
    for (size_t i = 0; i < m_numThreads; i++)
    {
        if (m_preliminaryResultsPool[i].size() == 0)
        {
            continue;
        }
        // I hate fucking bounds checking.
        // makes my if statements look like shit.
        for (const auto& r : m_preliminaryResultsPool[i])
        {
            if (extendConnected && m_resultsPool.size() > 0 &&
                m_resultsPool.back().relativeRegionStart +
                        m_resultsPool.back().regionSize ==
                    r.relativeRegionStart)
            {
                m_resultsPool.back().regionSize += r.regionSize;
            }
            else if (checkDuplicates && m_resultsPool.size() > 0 &&
                     m_resultsPool.back().relativeRegionStart ==
                         r.relativeRegionStart)
            {
                continue;
            }
            else
            {
                m_resultsPool.push_back(std::move(r));
            };
        }
    }
    return std::move(m_resultsPool);
}
