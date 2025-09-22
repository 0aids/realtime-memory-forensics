#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <sys/types.h>
#include <vector>
// A bit map of properties
namespace MemoryAnalysis::Properties {
enum e_Permissions {
  READ = 1 << 0,
  WRITE = 1 << 1,
  EXECUTE = 1 << 2,
  PRIVATE = 1 << 3,
};
typedef unsigned char PermissionsMask;

std::string maskToString(PermissionsMask perms);

PermissionsMask charsToMask(char perms[4]);

struct BasicRegionProperties {
  const std::string m_name;
  const uintptr_t m_start;
  const size_t m_size;
  const PermissionsMask m_perms;

  BasicRegionProperties(std::string name, uintptr_t start, size_t size,
                        PermissionsMask perms);

  std::string toStr();
  const std::string toConstStr() const;
};
struct MemoryRegionProperties;

struct MemorySubRegionProperties : public BasicRegionProperties {
public:
  std::weak_ptr<MemoryRegionProperties> parentRegion;
};

struct MemoryRegionProperties : public BasicRegionProperties {
public:
  MemoryRegionProperties(std::string name, uintptr_t start, size_t size,
                         PermissionsMask perms);
  std::vector<std::shared_ptr<MemorySubRegionProperties>> childSubRegions;
  //
};

using RegionPropertiesList = std::vector<MemoryRegionProperties>;

std::ostream &operator<<(std::ostream &os, BasicRegionProperties *properties);
} // namespace MemoryAnalysis::Properties
