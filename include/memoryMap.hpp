#include "memoryRegion.hpp"
#include <cstring>
#include <fstream>

class MemoryMap {
private:
  std::string mapFileLocation;
  RegionPropertiesList regionProperties;
  pid_t pid;

public:
  void readMaps() {
    std::ifstream memoryMapFile(this->mapFileLocation);
    std::string line;
    int unnamedRegionNumber = 1;

    while (std::getline(memoryMapFile, line)) {
      uintptr_t startAddr, endAddr;
      char permR, permW, permX, permP;
      char name[1024] = "";
      // DEBUG("Processing line: " << line);
      sscanf(line.c_str(), "%lx-%lx %c%c%c%c %*s %*s %*s %[^\n]", &startAddr,
             &endAddr, &permR, &permW, &permX, &permP, name);
      char permissions[4] = {permR, permW, permX, permP};
      if (strlen(name) == 0) {
        // DEBUG("UnnamedRegion: " << unnamedRegionNumber);
        std::string unnamedName =
            "UnnamedRegion-" + std::to_string(unnamedRegionNumber++);
        strncpy(name, unnamedName.c_str(), sizeof(name) - 1);
      }
      MemoryRegionProperties currentRegionProperties(
          startAddr, endAddr - startAddr, name,
          generatePermissionsMask(permissions), pid);
      // DEBUG("Current region properties: \n" << currentRegionProperties)
      this->regionProperties.push_back(currentRegionProperties);
    }
    memoryMapFile.close();
  }

  MemoryMap(pid_t pid) : mapFileLocation(mapLocationFromPID(pid)), pid(pid) {}

  MemoryMap() {}

  const RegionPropertiesList &getRegionProperties() { return regionProperties; }
};
