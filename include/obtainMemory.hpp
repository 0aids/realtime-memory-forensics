#pragma once
#include "logger.hpp"
#include "memoryMap.hpp"
#include "memoryRegion.hpp"
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <unordered_map>
extern "C" {
#include <fcntl.h>
#include <sched.h>
#include <sys/types.h>
#include <unistd.h>
}
#include <string>

typedef std::vector<MemoryRegion> MemoryRegionList;
typedef std::unordered_map<std::string, size_t> MapNameToIndex;

MemoryRegion NULLREGION;

class ProgramMemory {
private:
  MemoryMap map;
  MemoryRegionList regions;
  MapNameToIndex regionNameToIndex;
  pid_t pid;

public:
  ProgramMemory(pid_t pid) : pid(pid) {}
  ProgramMemory() {}

  void updateMemoryMap();
  bool checkMemoryMapUpdate();

  MemoryRegion &operator[](size_t index) {
    if (index >= regions.size()) {
      ERROR("Index: '" << index << "' is out of range for regions of amount: "
                       << regions.size());
      return NULLREGION;
    }

    return regions[index];
  }

  MemoryRegion &operator[](std::string regionName) {
    if (!this->regionNameToIndex.contains(regionName)) {
      ERROR("Region: '" << regionName << "' doesn't exist");
      return NULLREGION;
    }

    return regions[regionNameToIndex[regionName]];
  }

  // Read and update the memory for a certain region.
  void updateMemoryRegion(std::string regionName) {
    if (!this->regionNameToIndex.contains(regionName)) {
      ERROR("Region: '" << regionName << "' doesn't exist");
      return;
    }

    regions[regionNameToIndex[regionName]].updateBinaryStream();
    return;
  }

  /////////////////////////////////////////////////
  void setup() {
    if (!this->pid) {
      ERROR("Program memory PID has not been set!!!");
      exit(EXIT_FAILURE);
    }
    map = MemoryMap(this->pid);
    map.readMaps();

    auto regionProperties = this->map.getRegionProperties();
    this->regions.reserve(regionProperties.size());
    this->regionNameToIndex.reserve(regionProperties.size());
    size_t i = 0;

    for (auto &regionProperty : regionProperties) {
      regions.push_back(MemoryRegion(regionProperty));
      this->regionNameToIndex.emplace(regionProperty.regionName, i++);
    }
  }
  void printMemoryRegionListNames() {
    for (auto &memoryRegion : this->regions) {
      std::cout << memoryRegion.getProperties().regionName << std::endl;
    }
  }

  //////////////////////////////////////////////////////////
  MemoryRegion *searchFirst(std::string string) {
    // filter by readable, and then copy the corresponding memory
    // returns nullptr if doesn't exist.
    for (auto &region : this->regions) {
      auto regionPerms = region.getProperties().permissions;
      if (!isReadable(regionPerms) || !isWritable(regionPerms)) {
        continue;
      }

      // DEBUG("Reading region: " << region.getProperties());
      region.updateBinaryStream();
      auto location = region.findString(string);
      if (location != UINTPTR_MAX) {
        DEBUG("Found string @: " << std::showbase << std::hex << location);
        DEBUG("In region: " << region.getProperties());
        return &region;
      }
    }
    DEBUG("Didn't find string");
    return nullptr;
  }
};
