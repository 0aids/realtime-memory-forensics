#include "memory_region.hpp"
#include <memory>
#include <thread>
#include <utility>
#include "log.hpp"
#include "region_properties.hpp"
#include <chrono>
#include <cstdint>
#include <cstring>
#include <string>
extern "C" {
#include <bits/types/struct_iovec.h>
#include <sys/uio.h>
}

std::string RegionSnapshot::toStr() const {
    Log(Debug, "snapshottedTime: " << this->snapshottedTime);

    std::string str(this->size(), '.');
    Log(Debug, std::hex << std::showbase << "Size: " << this->size());
    for (size_t i = 0; i < this->size(); i++) {
        if (this->at(i) < 32)
            continue;

        str[i] = this->at(i);
    }
    return str;
}

MemoryRegion::MemoryRegion(MemoryRegionProperties properties,
                           pid_t                  pid) :
    m_pid(pid), m_regionProperties(properties) {
    // nothing
}

void MemoryRegion::snapshot() {
    using namespace std::chrono;
    const uintptr_t& startAddr =
        m_regionProperties.parentRegionStart +
        m_regionProperties.regionStart;
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
        "Start address: " << std::showbase << std::hex << startAddr);
    Log(Debug, "Size: " << std::showbase << std::hex << size);
    Log(Debug, "perms: " << permissionsMaskToString(perms));
    int64_t      totalBytesRead = 0;
    const size_t chunks         = 1 << 24;
    Log(Debug, "Chunk size: " << chunks);

    auto beforeTime = steady_clock::now().time_since_epoch();

    while (totalBytesRead < static_cast<int64_t>(size)) {
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
        if (nread <= 0) {
            if (nread == -1) {
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

SP_RegionSnapshot MemoryRegion::getLastSnapshot() const {
    return m_snapshots_l.back();
}

void RegionSnapshot::resetFailed() {
    failed = false;
}

bool RegionSnapshot::getFailed() {
    auto f = failed;
    resetFailed();
    return f;
}

std::vector<MemoryRegionProperties>
RegionSnapshot::findChangedRegions(const RegionSnapshot& otherRegion,
                                   uint32_t compareSize) const {
    std::vector<MemoryRegionProperties> changes = {};
    using namespace std::chrono;
    auto before = steady_clock::now().time_since_epoch();

    // if (this->regionProperties != otherRegion.regionProperties) {
    //     Log(Error,
    //         "Incomparable region snapshots; have different "
    //         "properties");
    //     this->failed = true;
    //     return changes;
    // }

    // Construct the pool's arguments.
    // A pair giving the start(inclusive) and end(exclusive).
    size_t numThreads = std::thread::hardware_concurrency() - 2;
    std::vector<std::array<size_t, 2>> perThreadArguments(numThreads);
    // ensure that we get the last guys as well.
    size_t                   regionSize = this->size() / numThreads;
    std::vector<std::thread> threads;
    std::vector<std::vector<MemoryRegionProperties>> results(
        numThreads);
    Log(Debug, "NumThreads: " << numThreads);

    for (size_t i = 0; i < numThreads; i++) {
        size_t start  = i * regionSize;
        size_t end    = (i != numThreads - 1) ? (i + 1) * regionSize :
                                                this->size();
        auto&  result = results[i];
        threads.push_back(std::thread([this, &otherRegion, start, end,
                                       &result, compareSize]() {
            size_t size = end - start;
            // Log(Debug, "Thread: " << i << " has started.");
            // Log(Debug,
            //     "Start: " << std::hex << std::showbase << start);
            // Log(Debug, "end: " << std::hex << std::showbase << end);
            size_t offset = start;
            for (size_t i = 0; i < size / compareSize - 1; ++i) {
                size_t csize = (i == (size / compareSize) - 1) ?
                    size - compareSize * (i - 1) :
                    compareSize;
                if (memcmp(this->data() + offset,
                           otherRegion.data() + offset,
                           compareSize)) {
                    if (result.size() > 0 &&
                        result.back().regionStart +
                                result.back().regionSize ==
                            offset) {
                        result.back().regionSize += csize;
                    } else {
                        auto newRegionProperties =
                            this->regionProperties;
                        newRegionProperties.regionSize  = csize;
                        newRegionProperties.regionStart = offset;
                        result.push_back(
                            std::move(newRegionProperties));
                    };
                }
                offset += csize;
            }
            // Log(Debug,
            //     "Thread: " << i << " has finished. Num changes: "
            //                << result.size() << "\tstart + offset: "
            //                << std::hex << std::showbase << offset);
        }));
    }

    size_t total = 0;
    for (size_t i = 0; i < numThreads; i++) {
        threads[i].join();
        total += results[i].size();
    }
    Log(Debug,
        "Total number of changes before consolidation: " << total);
    changes.reserve(total);

    for (size_t i = 0; i < numThreads; i++) {
        for (const auto& r : results[i]) {
            if (changes.size() > 0 &&
                changes.back().regionStart +
                        changes.back().regionSize ==
                    r.regionStart) {
                changes.back().regionSize += r.regionSize;
            } else {
                changes.push_back(std::move(r));
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
std::vector<MemoryRegionProperties>
RegionSnapshot::findUnchangedRegions(
    const RegionSnapshot& otherRegion, uint32_t compareSize) const {
    std::vector<MemoryRegionProperties> changes = {};

    // if (this->regionProperties != otherRegion.regionProperties) {
    //     Log(Error,
    //         "Incomparable region snapshots; have different "
    //         "properties");
    //     this->failed = true;
    //     return changes;
    // }

    for (size_t i = 0; i < this->size() / compareSize; ++i) {
        if (!memcmp(this->data() + (i * compareSize),
                    otherRegion.data() + (i * compareSize),
                    compareSize)) {
            if (changes.size() > 0 &&
                changes.back().regionStart +
                        changes.back().regionSize ==
                    i * compareSize) {
                changes.back().regionSize += compareSize;
            } else {
                auto newRegionProperties = this->regionProperties;
                newRegionProperties.regionSize  = compareSize;
                newRegionProperties.regionStart = i * compareSize;
                changes.push_back(std::move(newRegionProperties));
            };
        }
    }

    return changes;
}

std::vector<MemoryRegionProperties>
RegionSnapshot::findStringLikeRegions(const size_t& minLength) {
    // I'll just do a slow algorithm first. do it right then optimize later.
    std::vector<MemoryRegionProperties> stringLike;
    for (uintptr_t i = 0;
         i < static_cast<uintptr_t>(regionProperties.regionSize);
         i++) {
        uintptr_t memoryPointer = i;

        if (this->at(memoryPointer) - 31 > 0) {
            if (stringLike.size() > 0 &&
                stringLike.back().regionStart +
                        stringLike.back().regionSize ==
                    memoryPointer) {
                // Log(Message, "Found consecutive stringlike");
                stringLike.back().regionSize++;
            } else {
                // Log(Message, "Found new stringlike");
                // if (stringLike.size() > 0)
                //     Log(Debug,
                //         "I: " << i << "\tstart: "
                //               << stringLike.back().regionStart
                //               << "\tsize: "
                //               << stringLike.back().regionSize);
                auto rp        = this->regionProperties;
                rp.regionSize  = 1;
                rp.regionStart = i;
                stringLike.push_back(std::move(rp));
            }
        } else if (stringLike.size() > 0 &&
                   stringLike.back().regionSize < minLength) {
            // Log(Debug, "Popping back due to insufficient size");
            stringLike.pop_back();
        }
    }

    return stringLike;
}
