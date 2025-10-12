#include "memory_region.hpp"
#include "log.hpp"
#include "region_properties.hpp"
#include "analysis_threadpool.hpp"
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



// For 0xc0000000 bytes, comparison took 9.8 seconds ;) (with 10 threads and 8
// byte comparisons)
RegionPropertiesList
RegionSnapshot::findChangedRegions(const RegionSnapshot& otherRegion,
                                   const uint32_t compareSize) const
{
    RegionPropertiesList changes = {};
    using namespace std::chrono;
    auto before = steady_clock::now().time_since_epoch();

    if (otherRegion.size() != this->size() ||
        this->size() != regionProperties.regionSize)
    {
        Log(Error, "Incomparable regions of differing size!!!");
        return changes;
    }

    RegionAnalysisThreadPool tp(RegionAnalysisThreadPool::numThreads, 0, regionProperties);

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
RegionPropertiesList RegionSnapshot::findUnchangedRegions(
    const RegionSnapshot& otherRegion,
    const uint32_t        compareSize) const
{

    if (otherRegion.size() != this->size() ||
        this->size() != regionProperties.regionSize)
    {
        Log(Error, "Incomparable regions of differing size!!!");
        return RegionPropertiesList();
    }

    RegionAnalysisThreadPool tp(RegionAnalysisThreadPool::numThreads
                                , 0, regionProperties);

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

RegionPropertiesList
RegionSnapshot::findStringLikeRegions(const size_t& minLength)
{
    RegionAnalysisThreadPool tp(NUMTHREADS, minLength, regionProperties);

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

RegionPropertiesList RegionSnapshot::findOf(const std::string& str)
{
    RegionAnalysisThreadPool tp(NUMTHREADS, str.length(), regionProperties);

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

RegionPropertiesList
RegionSnapshot::findPointers(const uintptr_t& actualAddress)
{
    RegionAnalysisThreadPool tp(NUMTHREADS, ptrSize, regionProperties);

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

RegionPropertiesList RegionSnapshot::findPointerLikes()
{
    RegionAnalysisThreadPool tp(NUMTHREADS, 0,
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
RegionPropertiesList
RegionSnapshot::findLinkedList(const uintptr_t& memberAddress,
                               const uintptr_t& numHeaderBytes)
{
    RegionPropertiesList result;
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
    if (i >= maxIters)
    {
        Log(Warning,
            "Maximum linked list traversal reached, traversed: "
                << i << " times.");
    }

    return result;
}

RegionPropertiesList
RegionSnapshot::findDoubleLike(const double& lower,
                               const double& upper)
{
    RegionAnalysisThreadPool tp(NUMTHREADS, 0,
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
