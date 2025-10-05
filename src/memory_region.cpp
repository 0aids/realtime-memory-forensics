#include "memory_region.hpp"
#include "log.hpp"
#include "region_properties.hpp"
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <utility>
extern "C"
{
#include <bits/types/struct_iovec.h>
#include <sys/uio.h>
}
inline size_t numberThreads = std::thread::hardware_concurrency() - 2;

std::string   RegionSnapshot::toStr() const
{
    Log(Debug, "snapshottedTime: " << this->snapshottedTime);

    std::string str(this->size(), '.');
    Log(Debug, std::hex << std::showbase << "Size: " << this->size());
    for (size_t i = 0; i < this->size(); i++)
    {
        if (this->at(i) < 32)
            continue;

        str[i] = this->at(i);
    }
    return str;
}

MemoryRegion::MemoryRegion(MemoryRegionProperties properties,
                           pid_t                  pid) :
    m_pid(pid), m_regionProperties(properties)
{
    // nothing
}

void MemoryRegion::snapshot()
{
    using namespace std::chrono;
    const uintptr_t& startAddr =
        m_regionProperties.getActualRegionStart();
    const uint64_t&    size  = m_regionProperties.regionSize;
    const std::string& name  = m_regionProperties.parentRegionName;
    const auto&        perms = m_regionProperties.permissions;
    struct iovec       localIovec[1];
    struct iovec       sourceIovec[1];
    std::shared_ptr<RegionSnapshot> snap =
        std::make_shared<RegionSnapshot>(
            steady_clock::now().time_since_epoch(),
            m_regionProperties);

    snap->resize(size);

    Log(Debug, "Region name: " << name);
    Log(Debug,
        "Actual Start address: " << std::showbase << std::hex
                                 << startAddr);
    Log(Debug, "Size: " << std::showbase << std::hex << size);
    Log(Debug, "perms: " << permissionsMaskToString(perms));
    int64_t      totalBytesRead = 0;
    const size_t chunks         = 1 << 24;
    Log(Debug, "Chunk size: " << chunks);

    auto beforeTime = steady_clock::now().time_since_epoch();

    while (totalBytesRead < static_cast<int64_t>(size))
    {
        uint64_t bytesToRead =
            (size - totalBytesRead > (ssize_t)chunks) ?
            chunks :
            size - totalBytesRead;
        sourceIovec[0].iov_base = (void*)(startAddr + totalBytesRead);
        sourceIovec[0].iov_len  = bytesToRead;

        localIovec[0].iov_base = snap->data() + totalBytesRead;
        localIovec[0].iov_len  = bytesToRead;

        // This is a lot simpler than using lseek, and permissions are granted (no
        // sudo) if yama is 0
        ssize_t nread =
            process_vm_readv(m_pid, localIovec, 1, sourceIovec, 1, 0);
        if (nread <= 0)
        {
            if (nread == -1)
            {
                Log(Error,
                    "Completely failed to read the region. Error "
                    "is below.");
                perror("process_vm_readv");
                return;
            }
            Log(Error,
                "Read " << nread << "/" << size
                        << "bytes. Failed to read all the bytes "
                           "from that region.");

            return;
        }
        totalBytesRead += nread;
    }
    Log(Debug, "Read: " << totalBytesRead << " Bytes into m_bv");
    auto afterTime = steady_clock::now().time_since_epoch();
    Log(Message,
        "Reading took: "
            << duration_cast<milliseconds>(afterTime - beforeTime));
    beforeTime = steady_clock::now().time_since_epoch();
    this->m_snapshots_l.push_back(std::move(snap));
    afterTime = steady_clock::now().time_since_epoch();
    Log(Message,
        "Pushing back took: "
            << duration_cast<milliseconds>(afterTime - beforeTime));
    return;
}

SP_RegionSnapshot MemoryRegion::getLastSnapshot() const
{
    return m_snapshots_l.back();
}

void RegionSnapshot::resetFailed()
{
    failed = false;
}

bool RegionSnapshot::getFailed()
{
    auto f = failed;
    resetFailed();
    return f;
}

// For 0xc0000000 bytes, comparison took 72.5s
std::vector<MemoryRegionProperties>
RegionSnapshot::findChangedRegionsSingleThread(
    const RegionSnapshot& otherRegion, uint32_t compareSize) const
{
    using namespace std::chrono;
    auto before = steady_clock::now().time_since_epoch();
    std::vector<MemoryRegionProperties> changes = {};
    for (size_t i = 0; i < this->size() / compareSize; ++i)
    {
        if (memcmp(this->data() + (i * compareSize),
                   otherRegion.data() + (i * compareSize),
                   compareSize))
        {
            if (changes.size() > 0 &&
                changes.back().relativeRegionStart +
                        changes.back().regionSize ==
                    i * compareSize)
            {
                changes.back().regionSize += compareSize;
            }
            else
            {
                auto newRegionProperties = this->regionProperties;
                newRegionProperties.regionSize = compareSize;
                newRegionProperties.relativeRegionStart =
                    i * compareSize;
                changes.push_back(newRegionProperties);
            };
        }
    }

    Log(Message,
        "Time taken for finding changes: "
            << duration_cast<milliseconds>(
                   steady_clock::now().time_since_epoch() - before));
    Log(Message, "Number of bytes compared: " << this->size());
    return changes;
}

struct RegionAnalysisCapture
{
    uintptr_t start; // Start is relative and inclusive.
    uintptr_t size;  // ( start + size ) is not inclusive.
    std::vector<MemoryRegionProperties>& results;
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

class RegionAnalysisThreadPool
{
  private:
    size_t                            m_numThreads;
    size_t                            m_overlap;
    std::vector<RegionAnalysisThread> m_threadPool;
    size_t                            m_totalResults;
    const MemoryRegionProperties&     m_regionProperties;
    std::vector<std::vector<MemoryRegionProperties>>
         m_preliminaryResultsPool;

    bool m_complete = false;

  public:
    std::vector<MemoryRegionProperties> m_resultsPool;
    RegionAnalysisThreadPool(
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
    void startAllThreads(const RegionAnalysisFunction& func)
    {
        uintptr_t bytesPerThread =
            m_regionProperties.regionSize / m_numThreads;
        for (size_t i = 0; i < m_numThreads; i++)
        {
            RegionAnalysisCapture capture = {
                .start = m_regionProperties.relativeRegionStart +
                    i * bytesPerThread,
                .size    = bytesPerThread + m_overlap,
                .results = m_preliminaryResultsPool[i],
            };

            // Ensure we stay in bounds
            if (capture.start + capture.size >=
                m_regionProperties.regionSize)
            {
                capture.size =
                    m_regionProperties.regionSize - capture.start;
            }
            Log(Debug, "Thread: " << i << " has started.");
            Log(Debug,
                "Start: " << std::hex << std::showbase
                          << capture.start);
            Log(Debug,
                "end: " << std::hex << std::showbase
                        << capture.start + capture.size);

            m_threadPool.push_back(
                RegionAnalysisThread(capture, func));
        }
    }

    void joinAllThreads()
    {
        m_totalResults = 0;
        for (size_t i = 0; i < m_numThreads; i++)
        {
            m_threadPool[i].join();
            m_totalResults += m_preliminaryResultsPool[i].size();
        }
    }

    std::vector<MemoryRegionProperties>
    consolidateResults(bool extendConnected = false,
                       bool checkDuplicates = false)
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
                else if (checkDuplicates &&
                         m_resultsPool.size() > 0 &&
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
};

// For 0xc0000000 bytes, comparison took 9.8 seconds ;) (with 10 threads and 8
// byte comparisons)
std::vector<MemoryRegionProperties>
RegionSnapshot::findChangedRegions(const RegionSnapshot& otherRegion,
                                   const uint32_t compareSize) const
{
    std::vector<MemoryRegionProperties> changes = {};
    using namespace std::chrono;
    auto before = steady_clock::now().time_since_epoch();

    // Construct the pool's arguments.
    // A pair giving the start(inclusive) and end(exclusive).
    size_t numThreads = std::thread::hardware_concurrency() - 2;
    // Defer to single thread to reduce multithreading overhead on smaller
    // workloads
    //                  1 Mb
    if (this->size() < 1 << 20)
    {
        return this->findChangedRegionsSingleThread(otherRegion,
                                                    compareSize);
    }

    RegionAnalysisThreadPool tp(numThreads, 0, regionProperties);

    tp.startAllThreads(
        [&compareSize, &otherRegion,
         this](RegionAnalysisCapture capture)
        {
            size_t size    = capture.size;
            size_t current = capture.start;
            for (size_t i = 0; i < size / compareSize; ++i)
            {
                size_t csize = (i == (size / compareSize) - 1) ?
                    (capture.start + capture.size) - current :
                    compareSize;
                if (memcmp(this->data() + current,
                           otherRegion.data() + current, csize))
                {
                    if (capture.results.size() > 0 &&
                        capture.results.back().relativeRegionStart +
                                capture.results.back().regionSize ==
                            current)
                    {
                        capture.results.back().regionSize += csize;
                    }
                    else
                    {
                        auto newRegionProperties =
                            this->regionProperties;
                        newRegionProperties.regionSize = csize;
                        newRegionProperties.relativeRegionStart =
                            current;
                        capture.results.push_back(
                            std::move(newRegionProperties));
                    };
                }
                current += csize;
            }
        });
    tp.joinAllThreads();
    changes = tp.consolidateResults(true);

    Log(Message,
        "Time taken for finding changes: "
            << duration_cast<milliseconds>(
                   steady_clock::now().time_since_epoch() - before));
    Log(Message, "Number of bytes compared: " << this->size());

    return changes;
}
std::vector<MemoryRegionProperties>
RegionSnapshot::findUnchangedRegions(
    const RegionSnapshot& otherRegion,
    const uint32_t        compareSize) const
{
    std::vector<MemoryRegionProperties> changes = {};

    // if (this->regionProperties != otherRegion.regionProperties) {
    //     Log(Error,
    //         "Incomparable region snapshots; have different "
    //         "properties");
    //     this->failed = true;
    //     return changes;
    // }

    for (size_t i = 0; i < this->size() / compareSize; ++i)
    {
        if (!memcmp(this->data() + (i * compareSize),
                    otherRegion.data() + (i * compareSize),
                    compareSize))
        {
            if (changes.size() > 0 &&
                changes.back().relativeRegionStart +
                        changes.back().regionSize ==
                    i * compareSize)
            {
                changes.back().regionSize += compareSize;
            }
            else
            {
                auto newRegionProperties = this->regionProperties;
                newRegionProperties.regionSize = compareSize;
                newRegionProperties.relativeRegionStart =
                    i * compareSize;
                changes.push_back(std::move(newRegionProperties));
            };
        }
    }

    return changes;
}

std::vector<MemoryRegionProperties>
RegionSnapshot::findStringLikeRegions(const size_t& minLength)
{
    RegionAnalysisThreadPool tp(
        (this->size() < 1 << 20) ? 1 : numberThreads, minLength,
        regionProperties);

    tp.startAllThreads(
        [this, minLength](RegionAnalysisCapture cap)
        {
            for (uintptr_t i = 0; i < cap.size; i++)
            {
                uintptr_t memoryPointer = i;
                // Log(Debug, "I: " << i);
                // Log(Debug, "cap: " << cap.size);

                if (this->at(memoryPointer) - 31 > 0)
                {
                    if (cap.results.size() > 0 &&
                        cap.results.back().relativeRegionStart +
                                cap.results.back().regionSize ==
                            memoryPointer)
                    {
                        // Log(Message, "Found consecutive stringlike");
                        cap.results.back().regionSize++;
                    }
                    else
                    {
                        // Log(Message, "Found new stringlike");
                        // if (stringLike.size() > 0)
                        //     Log(Debug,
                        //         "I: " << i << "\tstart: "
                        //               << stringLike.back().regionStart
                        //               << "\tsize: "
                        //               << stringLike.back().regionSize);
                        auto rp       = this->regionProperties;
                        rp.regionSize = 1;
                        rp.relativeRegionStart = i;
                        cap.results.push_back(std::move(rp));
                    }
                }
                else if (cap.results.size() > 0 &&
                         cap.results.back().regionSize < minLength)
                {
                    // Log(Debug, "Popping back due to insufficient size");
                    cap.results.pop_back();
                }
            }
        });

    tp.joinAllThreads();
    return tp.consolidateResults(true, true);
}

std::vector<MemoryRegionProperties>
RegionSnapshot::findOf(const std::string& str)
{
    RegionAnalysisThreadPool tp(
        (this->size() < 1 << 20) ? 1 : numberThreads, str.length(),
        regionProperties);

    tp.startAllThreads(
        [&str, this](RegionAnalysisCapture cap)
        {
            for (uintptr_t i = cap.start;
                 i < cap.start + cap.size - str.length(); i++)
            {
                size_t count = 0;
                for (uintptr_t j = 0; j < str.length(); j++)
                {
                    if ((*this)[i + j] != str[j])
                    {
                        break;
                    }
                    count++;
                }
                if (count == str.length())
                {
                    MemoryRegionProperties newRegion =
                        regionProperties;
                    newRegion.relativeRegionStart = i;
                    newRegion.regionSize          = str.length();
                    cap.results.push_back(newRegion);
                }
            }
        });

    tp.joinAllThreads();
    return tp.consolidateResults();
}
