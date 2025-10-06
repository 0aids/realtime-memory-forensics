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
inline size_t NUMTHREADS = std::thread::hardware_concurrency() - 2;

static inline size_t niceAmountOfThreads(uintptr_t size)
{
    return (size < 1 << 20) ? 1 : NUMTHREADS;
}

std::string RegionSnapshot::toStr() const
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

bool MemoryRegion::snapshot()
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
                return false;
            }
            Log(Error,
                "Read " << nread << "/" << size
                        << "bytes. Failed to read all the bytes "
                           "from that region.");

            return false;
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
    return true;
}

SP_RegionSnapshot MemoryRegion::getLastSnapshot() const
{
    return m_snapshots_l.back();
}

// For 0xc0000000 bytes, comparison took 72.5s at 8byte comparisons.
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
    // Relative to the subregion, not to the actual.
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
        // Align to 64 bits. Allows for proper alignment for certain Data structures.
        uintptr_t bytesPerThread =
            ((m_regionProperties.regionSize / m_numThreads) / 8) * 8;
        Log(Debug, "Bytes per thread: " << bytesPerThread);
        Log(Debug, "Overlap: " << m_overlap);
        for (size_t i = 0; i < m_numThreads; i++)
        {
            RegionAnalysisCapture capture = {
                // Relative to the sub region.
                .start   = i * bytesPerThread,
                .size    = bytesPerThread + m_overlap,
                .results = m_preliminaryResultsPool[i],
            };

            // Stupid fucking bounds
            // BUG: Fix the stupid error with funky ass small regions.
            // I do not have the energy to figure out if capture.start is
            // Relative to the parent region or to the actual snapshotting region.
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
                "size: " << std::hex << std::showbase
                         << capture.size);
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

    if (otherRegion.size() != this->size() ||
        this->size() != regionProperties.regionSize)
    {
        Log(Error, "Incomparable regions of differing size!!!");
        return changes;
    }

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
    size_t numThreads = NUMTHREADS;
    // Defer to single thread to reduce multithreading overhead on smaller
    // workloads
    //                  1 Mb
    if (this->size() < 1 << 20)
    {
        numThreads = 1;
    }

    if (otherRegion.size() != this->size() ||
        this->size() != regionProperties.regionSize)
    {
        Log(Error, "Incomparable regions of differing size!!!");
        return std::vector<MemoryRegionProperties>();
    }

    RegionAnalysisThreadPool tp(numThreads, 0, regionProperties);

    tp.startAllThreads(
        [&compareSize, &otherRegion,
         this](RegionAnalysisCapture capture)
        {
            for (size_t i = 0; i < capture.size / compareSize; ++i)
            {
                if (!memcmp(this->data() + (i * compareSize),
                            otherRegion.data() + (i * compareSize),
                            compareSize))
                {
                    if (capture.results.size() > 0 &&
                        capture.results.back().relativeRegionStart +
                                capture.results.back().regionSize ==
                            i * compareSize)
                    {
                        capture.results.back().regionSize +=
                            compareSize;
                    }
                    else
                    {
                        auto newRegionProperties =
                            this->regionProperties;
                        newRegionProperties.regionSize = compareSize;
                        newRegionProperties.relativeRegionStart =
                            i * compareSize;
                        capture.results.push_back(
                            std::move(newRegionProperties));
                    };
                }
            }
        });

    tp.joinAllThreads();
    // Check duplicates is only really for overlaps.
    return tp.consolidateResults(true, false);
}

std::vector<MemoryRegionProperties>
RegionSnapshot::findStringLikeRegions(const size_t& minLength)
{
    RegionAnalysisThreadPool tp((this->size() < 1 << 20) ? 1 :
                                                           NUMTHREADS,
                                minLength, regionProperties);

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
    RegionAnalysisThreadPool tp((this->size() < 1 << 20) ? 1 :
                                                           NUMTHREADS,
                                str.length(), regionProperties);

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
constexpr static size_t ptrSize    = sizeof(uintptr_t);
constexpr static size_t doubleSize = sizeof(double);

std::vector<MemoryRegionProperties>
RegionSnapshot::findPointers(const uintptr_t& actualAddress)
{
    RegionAnalysisThreadPool tp(niceAmountOfThreads(this->size()),
                                ptrSize, regionProperties);

    Log(Debug,
        "Looking for address: " << std::hex << std::showbase
                                << actualAddress);

    tp.startAllThreads(
        [&actualAddress, this](RegionAnalysisCapture cap)
        {
            for (uintptr_t i = cap.start;
                 i < cap.start + cap.size - ptrSize; i += ptrSize)
            {
                uintptr_t value;
                memcpy(&value, this->data() + i, ptrSize);
                if (value == actualAddress)
                {
                    auto res                = this->regionProperties;
                    res.regionSize          = ptrSize;
                    res.relativeRegionStart = i;
                    cap.results.push_back(std::move(res));
                }
            }
        });

    tp.joinAllThreads();
    return tp.consolidateResults(false, true);
}

std::vector<MemoryRegionProperties> RegionSnapshot::findPointerLikes()
{
    RegionAnalysisThreadPool tp(niceAmountOfThreads(this->size()), 0,
                                regionProperties);

    Log(Debug,
        "Searching for ptrlike structures, anything that points to "
        "something within this region.");
    uintptr_t low  = regionProperties.getActualRegionStart();
    uintptr_t high = low + regionProperties.regionSize;
    Log(Debug, "Low: " << std::hex << std::showbase << low);
    Log(Debug, "High: " << std::hex << std::showbase << high);

    tp.startAllThreads(
        [&low, &high, this](RegionAnalysisCapture cap)
        {
            for (uintptr_t i = 0; i < cap.size; i += ptrSize)
            {
                uintptr_t offset = i + cap.start;
                uintptr_t value;
                memcpy(&value, this->data() + offset, ptrSize);
                if (value < high && value >= low)
                {
                    auto propertiesCopy = this->regionProperties;
                    propertiesCopy.relativeRegionStart = offset;
                    propertiesCopy.regionSize          = ptrSize;
                    cap.results.push_back(std::move(propertiesCopy));
                }
            }
        });
    tp.joinAllThreads();
    return tp.consolidateResults(false, false);
}
std::vector<MemoryRegionProperties>
RegionSnapshot::findLinkedList(const uintptr_t& memberAddress,
                               const uintptr_t& numHeaderBytes)
{
    std::vector<MemoryRegionProperties> result;
    // We are going to be generating a shit tonne of annoyign messages, so disable them.
    auto oldLogLevel      = Logger::m_logLevel;
    Logger::m_logLevel    = Message;
    uintptr_t addr        = memberAddress;
    size_t    numPointers = 0;
    size_t    i           = 0;
    size_t    maxIters    = 100;
    do
    {
        auto ptrs   = this->findPointers(addr - numHeaderBytes);
        numPointers = ptrs.size();
        if (numPointers > 1)
        {
            Log(Warning,
                "Found more than 1 pointer linking back, only using "
                "the first one found.");
        }
        if (numPointers > 0)
        {
            result.push_back(ptrs.front());
            addr = ptrs.front().getActualRegionStart();
        }
        i++;
        // Ensure we don't fucking kill ourselves over a cyclic loop.
    } while (numPointers > 0 && i < maxIters);
    Logger::m_logLevel = oldLogLevel;

    return result;
}
std::vector<MemoryRegionProperties>
RegionSnapshot::findDoubleLike(const double& lower,
                               const double& upper)
{
    RegionAnalysisThreadPool tp(niceAmountOfThreads(this->size()), 0,
                                regionProperties);

    tp.startAllThreads(
        [this, &lower, &upper](RegionAnalysisCapture cap)
        {
            for (uintptr_t i = cap.start; i < cap.start + cap.size;
                 i += doubleSize)
            {
                double d;
                memcpy(&d, this->data() + i, doubleSize);
                if (d > lower && d < upper)
                {
                    MemoryRegionProperties a = regionProperties;
                    a.regionSize             = doubleSize;
                    a.relativeRegionStart    = i;
                    cap.results.push_back(a);
                }
            }
        });

    tp.joinAllThreads();
    return tp.consolidateResults(false, false);
}
