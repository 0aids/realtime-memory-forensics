#pragma once
#include "permissions.hpp"
#include <cstdint>
#include <sstream>

extern "C" {
#include <bits/types/struct_iovec.h>
#include <fcntl.h>
#include <sys/ptrace.h>
#include <sys/uio.h>
#include <unistd.h>
}

// It's not supposed to be changing itself, as this is a composited class
// (composited by MemoryRegion) So it should be fine. It get's changed by it's
// "parent" class.
struct MemoryRegionProperties {
  uintptr_t start;
  size_t size;
  PermissionsMask permissions;
  std::string regionName;
  pid_t pid;

  MemoryRegionProperties(uintptr_t start, size_t size, std::string regionName,
                         PermissionsMask permissions, pid_t pid)
      : start(start), size(size), permissions(permissions),
        regionName(regionName), pid(pid) {}
  MemoryRegionProperties() {};

  // No need for virtual we're just overriding
  std::string toStr() {
    std::stringstream res;
    res << "\tName: " + this->regionName + "\n";
    res << "\tStart: " << std::showbase << std::hex << this->start << +"\n";
    res << "\tSize: " << std::showbase << std::hex << this->size << +"\n";
    res << "\tPermissions: " + ungeneratePermissionsMask(this->permissions) +
               "\n";
    res << "\tPID: " << std::dec << pid << "\n\n";
    return res.str();
  }
};

class MemorySubRegionProperties : public MemoryRegionProperties {
private:
  MemoryRegionProperties *m_parentRegion_p;

public:
  MemorySubRegionProperties(MemoryRegionProperties *parentRegion_p,
                            uintptr_t start, size_t size)
      // Absolutely disgusting initializer list.
      : MemoryRegionProperties(start, size, parentRegion_p->regionName,
                               parentRegion_p->permissions,
                               parentRegion_p->pid),
        //
        m_parentRegion_p(parentRegion_p) {};

  std::string toStr() {
    std::stringstream res;
    res << "\tName (Subregion): " + this->regionName + "\n";
    res << "\tStart: " << std::showbase << std::hex << this->start << +"\n";
    res << "\tSize: " << std::showbase << std::hex << this->size << +"\n";
    res << "\tPermissions: " + ungeneratePermissionsMask(this->permissions) +
               "\n";
    res << "\tPID: " << std::dec << pid << "\n\n";
    return res.str();
  }
};

std::ostream &operator<<(std::ostream &os, MemoryRegionProperties &mrp) {
  os << mrp.toStr();
  return os;
}
