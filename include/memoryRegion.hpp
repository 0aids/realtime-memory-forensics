#pragma once
#include "logger.hpp"
#include "regionProperties.hpp"
#include <chrono>
#include <cstdint>
#include <memory>
#include <vector>

extern "C" {
#include <bits/types/struct_iovec.h>
#include <fcntl.h>
#include <sys/ptrace.h>
#include <sys/uio.h>
#include <unistd.h>
}

typedef std::vector<char> BinaryStream;

static inline auto GET_TIME_MS() {
  return (std::chrono::duration_cast<std::chrono::milliseconds>(
              std::chrono::system_clock::now().time_since_epoch())
              .count());
}

struct RegionSnapshot {
public:
  const BinaryStream bs;
  const uint64_t ms;

  RegionSnapshot(const BinaryStream &bs) : bs(bs), ms(GET_TIME_MS()) {};
};

typedef std::vector<RegionSnapshot> RegionSnapshotList;

class basicMemoryRegion {
protected:
  // Has to be pointer for polymorphism ;_;
  std::shared_ptr<MemoryRegionProperties> *m_properties;
  RegionSnapshotList m_snapshots;

public:
  basicMemoryRegion(std::shared_ptr<MemoryRegionProperties> *properties)
      : m_properties(properties) {};
  basicMemoryRegion() : m_properties() {};

public:
  void snapshot() {
    const pid_t &pid = this->m_properties->pid;
    const uintptr_t &startAddr = this->m_properties->start;
    const ssize_t &size = this->m_properties->size;
    // Construct the binarystream separately
    BinaryStream bs(size);

    struct iovec localIovec[1];
    struct iovec sourceIovec[1];
    // source - Where we are getting the data from
    sourceIovec[0].iov_base = (void *)startAddr;
    sourceIovec[0].iov_len = size;

    // Destionation - Our binstream type.
    localIovec[0].iov_base = bs.data();
    localIovec[0].iov_len = size;

    // This is a lot simpler than using lseek, and permissions are granted (no
    // sudo) if yama is 0
    ssize_t nread = process_vm_readv(pid, localIovec, 1, sourceIovec, 1, 0);
    if (nread < size) {
      ERROR("Failed to read entire region!");
      ERROR("Number of bytes successfully read: " << nread);
      ERROR("Region at fault: \n" << this->m_properties);
      if (nread == -1) {
        perror("process_vm_readv");
      }
      return;
    }
    this->m_snapshots.push_back(RegionSnapshot(bs));
    return;
  }

public:
  void clearSnapshotList() {
    // Should hopefully call all the destructors for each element.
    this->m_snapshots.clear();
  }

public:
  // Returns UINTPTR_MAX if could not find string.
  // Searches the last snapshot if available.
  uintptr_t findString(const std::string &string) {
    auto strSize = string.length();
    if (!this->m_snapshots.size()) {
      ERROR("No snapshots have been taken");
      return UINTPTR_MAX;
    }

    for (uintptr_t i = 0; i < this->m_snapshots.back().bs.size() - strSize - 1;
         i++) {
      uintptr_t j;
      for (j = 0; j < strSize; j++) {
        if (this->m_snapshots.back().bs[i + j] != string[j]) {
          break;
        }
      }
      if (j == strSize) {
        return i;
      }
    }
    return UINTPTR_MAX;
  }
  MemoryRegionProperties &getProperties() { return *this->m_properties; }
};

// TODO:
// Make memory region:
// Inherit from an abstract base class basicMemoryRegion. Then use the abstract
// base class to composite in vector of tracked regions, using the abstract
// class to define a subregion of memory, using the MemorySubRegionProperties
// instead.
class MemorySubRegion : public basicMemoryRegion {
public:
  MemorySubRegion(MemorySubRegionProperties *subProperties)
      : basicMemoryRegion(subProperties) {};
};

class MemoryRegion : public basicMemoryRegion {
private:
public:
  // No reference - We do not want to autoupdate as it could have unintended
  // consequences.
  MemoryRegion(MemoryRegionProperties *properties)
      : basicMemoryRegion(properties) {};
  MemoryRegion() {};
};
