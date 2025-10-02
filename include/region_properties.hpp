#pragma once
#include <cstdint>
#include <string_view>
#include <memory>
#include <string>
#include <sys/types.h>
#include <vector>
// A bit map of properties
enum e_Permissions {
    READ    = 1 << 0,
    WRITE   = 1 << 1,
    EXECUTE = 1 << 2,
    PRIVATE = 1 << 3,
};
// Change back to uint8_t, but you can't print it because this game is stupid
using PermissionsMask = uint32_t;

std::string     permissionsMaskToString(const PermissionsMask& perms);

PermissionsMask toPermissionsMask(const char perms[4]);
PermissionsMask toPermissionsMask(const std::string& chars);
PermissionsMask toPermissionsMask(const std::string_view& chars);

struct MemoryRegionProperties {
  public:
    // Should this be const? yes.
    const std::string     parentRegionName;
    const uintptr_t       parentRegionStart;
    const size_t          parentRegionSize;
    const PermissionsMask permissions;

  public:
    // The region start and size is relative to the parent region.
    uintptr_t regionStart;
    // The region start and size is relative to the parent region.
    size_t regionSize;

  public:
    MemoryRegionProperties(std::string name, uintptr_t start,
                           size_t size, PermissionsMask perms);

    std::string       toStr();
    const std::string toConstStr() const;

    auto operator<=>(const MemoryRegionProperties&) const = default;
};

using RegionPropertiesList = std::vector<MemoryRegionProperties>;

std::ostream& operator<<(std::ostream&                 os,
                         const MemoryRegionProperties& properties);
