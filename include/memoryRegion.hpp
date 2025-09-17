#pragma once
#include "logger.hpp"
#include "permissions.hpp"
#include <cstdint>
#include <sstream>
#include <vector>

extern "C" {
#include <fcntl.h>
#include <unistd.h>
}

typedef std::vector<char> BinaryStream;

class MemoryRegionProperties {
public:
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

std::ostream &operator<<(std::ostream &os, MemoryRegionProperties &mrp) {
  os << mrp.toStr();
  return os;
}

typedef std::vector<MemoryRegionProperties> RegionPropertiesList;
// Implement sorting later

class MemoryRegion {
private:
  MemoryRegionProperties properties;
  BinaryStream binaryStream;

public:
  // Takes a reference - Auto updates when the other map updates.
  MemoryRegion(MemoryRegionProperties &properties) : properties(properties) {
    binaryStream.resize(this->properties.size);
  };
  MemoryRegion() {}
  MemoryRegionProperties &getProperties() { return this->properties; }

  void updateBinaryStream() {
    //
    const std::string memLocation = memLocationFromPID(this->properties.pid);
    int memFileDescriptor = open(memLocation.c_str(), O_RDONLY);
    // DEBUG("Current memoryRegion: " << this->properties);

    if (memFileDescriptor == -1) {
      ERROR("Could not open file: " << memLocation);
      close(memFileDescriptor);
      ERROR("Region at fault: " << this->properties);
      return;
    }
    if (lseek(memFileDescriptor, this->properties.start, SEEK_SET) == -1) {
      ERROR("Could not seek to address: " << std::hex
                                          << this->properties.start);
      ERROR("Region at fault: " << this->properties);
      close(memFileDescriptor);
      return;
    }
    ssize_t bytes_read =
        read(memFileDescriptor, binaryStream.data(), this->properties.size);
    close(memFileDescriptor);

    if (bytes_read == -1) {
      ERROR("Could not read from memory");
      ERROR("Region at fault: " << this->properties);
      return;
    }
    return;
  }

  // Returns UINTPTR_MAX if could not find string.
  uintptr_t findString(const std::string &string) {
    auto strSize = string.length();

    for (uintptr_t i = 0; i < this->binaryStream.size() - strSize - 1; i++) {
      uintptr_t j;
      for (j = 0; j < strSize; j++) {
        if (this->binaryStream[i + j] != string[j]) {
          break;
        }
      }
      if (j == strSize) {
        return i;
      }
    }
    return UINTPTR_MAX;
  }

  // Convert to a nice show of hex values
  std::string formatMemoryContents() {}
};
