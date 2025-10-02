#include "memory_region.hpp"
#include <utility>
#include "log.hpp"
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
    m_regionProperties(properties), m_pid(pid) {
    // nothing
}

void MemoryRegion::snapshot() {
    using namespace std::chrono;
    const uintptr_t& startAddr =
        m_regionProperties.parentRegionStart +
        m_regionProperties.regionStart;
    const ssize_t&     size  = m_regionProperties.regionSize;
    const std::string& name  = m_regionProperties.parentRegionName;
    const auto&        perms = m_regionProperties.permissions;
    struct iovec       localIovec[1];
    struct iovec       sourceIovec[1];
    RegionSnapshot     snap{.snapshottedTime =
                            steady_clock::now().time_since_epoch(),
                            .regionProperties = m_regionProperties};

    snap.resize(size);

    Log(Debug, "Region name: " << name);
    Log(Debug,
        "Start address: " << std::showbase << std::hex << startAddr);
    Log(Debug, "Size: " << std::showbase << std::hex << size);
    Log(Debug, "perms: " << permissionsMaskToString(perms));
    ssize_t      totalBytesRead = 0;
    const size_t chunks         = 1 << 20;
    Log(Debug, "Chunk size: " << chunks);

    while (totalBytesRead < size) {
        size_t bytesToRead =
            (size - totalBytesRead > (ssize_t)chunks) ?
            chunks :
            size - totalBytesRead;
        sourceIovec[0].iov_base = (void*)(startAddr + totalBytesRead);
        sourceIovec[0].iov_len  = bytesToRead;

        localIovec[0].iov_base = snap.data() + totalBytesRead;
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
    this->m_snapshots_l.push_back(snap);
    return;
}

const RegionSnapshot& MemoryRegion::getLastSnapshot() {
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
                                   uint32_t compareSize) {
    std::vector<MemoryRegionProperties> changes = {};

    if (this->regionProperties != otherRegion.regionProperties) {
        Log(Error,
            "Incomparable region snapshots; have different "
            "properties");
        this->failed = true;
        return changes;
    }

    for (size_t i = 0; i < this->size() / compareSize; ++i) {
        if (memcmp(this->data() + (i * compareSize),
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
                changes.push_back(newRegionProperties);
            };
        }
    }

    return changes;
}
std::vector<MemoryRegionProperties>
RegionSnapshot::findUnchangedRegions(
    const RegionSnapshot& otherRegion, uint32_t compareSize) {
    std::vector<MemoryRegionProperties> changes = {};

    if (this->regionProperties != otherRegion.regionProperties) {
        Log(Error,
            "Incomparable region snapshots; have different "
            "properties");
        this->failed = true;
        return changes;
    }

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
                changes.push_back(newRegionProperties);
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
                Log(Message, "Found consecutive stringlike");
                stringLike.back().regionSize++;
            } else {
                Log(Message, "Found new stringlike");
                // if (stringLike.size() > 0)
                //     Log(Debug,
                //         "I: " << i << "\tstart: "
                //               << stringLike.back().regionStart
                //               << "\tsize: "
                //               << stringLike.back().regionSize);
                auto rp        = this->regionProperties;
                rp.regionSize  = 1;
                rp.regionStart = i;
                stringLike.push_back(rp);
            }
        } else if (stringLike.size() > 0 &&
                   stringLike.back().regionSize < minLength) {
            Log(Debug, "Popping back due to insufficient size");
            stringLike.pop_back();
        }
    }

    return stringLike;
}
