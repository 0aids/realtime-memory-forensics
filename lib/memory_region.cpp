#include "memory_region.hpp"
#include "globals.hpp"
#include "log.hpp"
#include "region_properties.hpp"
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
extern "C" {
#include <bits/types/struct_iovec.h>
#include <sys/uio.h>
}

static inline auto GET_TIME_MS() {
  return (std::chrono::duration_cast<std::chrono::milliseconds>(
              std::chrono::system_clock::now().time_since_epoch())
              .count());
}

namespace MemoryAnalysis {
BasicMemoryRegion::BasicMemoryRegion(
    const Properties::BasicRegionProperties *properties)
    : m_properties_p(properties) {};

e_ErrorTypes BasicMemoryRegion::snapshot(const pid_t &pid) {
  const uintptr_t &startAddr = this->m_properties_p->m_start;
  const ssize_t &size = this->m_properties_p->m_size;
  const std::string &name = this->m_properties_p->m_name;
  const auto &perms = this->m_properties_p->m_perms;
  struct iovec localIovec[1];
  struct iovec sourceIovec[1];
  RegionSnapshot snap(GET_TIME_MS());

  snap.resize(size);

  p_log_debug("");
  p_log_var(name);
  p_log_var_hex(startAddr);
  p_log_var_hex(size);
  p_log_debug("perms: " << Properties::maskToString(perms));

  sourceIovec[0].iov_base = (void *)startAddr;
  sourceIovec[0].iov_len = size;

  localIovec[0].iov_base = snap.data();
  localIovec[0].iov_len = size;

  // This is a lot simpler than using lseek, and permissions are granted (no
  // sudo) if yama is 0
  ssize_t nread = process_vm_readv(pid, localIovec, 1, sourceIovec, 1, 0);
  if (nread < size) {
    if (nread == -1) {
      p_log_error("Completely failed to read the region. Error is below.");
      perror("process_vm_readv");
      return ERROR;
    }
    p_log_error(
        "Read " << nread << "/" << size
                << "bytes. Failed to read all the bytes from that region.");

    return ERROR;
  }
  p_log_debug("Read: " << nread << " Bytes into m_bv");
  this->m_snaps_l.push_back(snap);
  return SUCCESS;
}

Properties::BasicRegionProperties BasicMemoryRegion::getProperties() {
  return *m_properties_p;
}

std::vector<uintptr_t> BasicMemoryRegion::getChanged(uintptr_t alignment) {
  // Be careful with alignment here
  std::vector<uintptr_t> changes = {};
  const auto &snapshot1 = this->m_snaps_l.back();
  const auto &snapshot2 = this->m_snaps_l[this->m_snaps_l.size() - 2];

  for (size_t i = 0; i < snapshot1.size() / alignment; ++i) {
    if (memcmp(snapshot1.data() + (i * alignment),
               snapshot2.data() + (i * alignment), alignment)) {
      changes.push_back(i * alignment);
    }
  }

  return changes;
}

std::vector<uintptr_t> BasicMemoryRegion::getUnchanged(uintptr_t alignment) {
  std::vector<uintptr_t> changes;
  if (this->m_snaps_l.size() < 2) {
    p_log_error("Not enough snapshots to perform comparison!");
    exit(1);
  }
  const auto &snapshot1 = this->m_snaps_l.back();
  const auto &snapshot2 = this->m_snaps_l[this->m_snaps_l.size() - 2];
  for (size_t i = 0; i < snapshot1.size() / alignment; ++i) {
    if (!memcmp(snapshot1.data() + (i * alignment),
                snapshot2.data() + (i * alignment), alignment)) {
      changes.push_back(i * alignment);
    }
  }

  return changes;
}

std::string RegionSnapshot::toStr() {
  p_log_debug("snapshottedTime: " << this->m_snapshottedTime);

  std::string str(this->size(), '.');
  p_log_var_hex(this->size());
  for (size_t i = 0; i < this->size(); i++) {
    if (this->at(i) < 32)
      continue;

    str[i] = this->at(i);
  }
  return str;
}

} // namespace MemoryAnalysis
