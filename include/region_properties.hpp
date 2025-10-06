#pragma once
#include <cstdint>
#include <string_view>
#include <memory>
#include <string>
#include <sys/types.h>
#include <vector>
// A bit map of properties
enum e_Permissions
{
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

struct MemoryRegionProperties
{
  public:
    // Should this be const? yes.
    // const fucking deletes the copy and move assignment operators?
    const std::string     parentRegionName  = "NULL";
    const uintptr_t       parentRegionStart = 0;
    const uint64_t        parentRegionSize  = 0;
    const PermissionsMask permissions       = 0;

  public:
    // Relative to the parent region
    uintptr_t relativeRegionStart;
    uintptr_t regionSize;

  public:
    MemoryRegionProperties() = default;
    MemoryRegionProperties(std::string name, uintptr_t start,
                           size_t size, PermissionsMask perms);
    ~MemoryRegionProperties()                             = default;
    MemoryRegionProperties(const MemoryRegionProperties&) = default;
    MemoryRegionProperties(MemoryRegionProperties&&)      = default;

    uintptr_t getActualRegionStart() const
    {
        return relativeRegionStart + parentRegionStart;
    }

    std::string       toStr();
    const std::string toConstStr() const;

    auto operator<=>(const MemoryRegionProperties&) const = default;
};

class RegionPropertiesList
    : public std::vector<MemoryRegionProperties>
{
  public:
    RegionPropertiesList
    getRegionsWithPermissions(const std::string_view& chars);
    RegionPropertiesList
    getRegionsWithPermissions(const PermissionsMask& mask);

    // Gets the first memory region with the corresponding name.
    // If it doesn't exist, will return empty optional.
    std::optional<MemoryRegionProperties>
    getRegionWithName(const std::string_view& name);
};

std::ostream& operator<<(std::ostream&                 os,
                         const MemoryRegionProperties& properties);
