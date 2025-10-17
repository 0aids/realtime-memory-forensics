#ifndef maps_hpp_INCLUDED
#define maps_hpp_INCLUDED

#include <memory_resource>
#include <ostream>
#include <string_view>
#include <cstdint>
#include <vector>
#include <unistd.h>

enum class Perms : uint8_t
{
    EMPTY   = 0,
    READ    = 1,
    WRITE   = 2,
    EXECUTE = 4,
    PRIVATE = 8,
    SHARED  = 16,
};

inline Perms operator|(Perms lhs, Perms rhs)
{
    return static_cast<Perms>(static_cast<uint8_t>(lhs) |
                              static_cast<uint8_t>(rhs));
}

inline Perms operator&(Perms lhs, Perms rhs)
{
    return static_cast<Perms>(static_cast<uint8_t>(lhs) &
                              static_cast<uint8_t>(rhs));
}

inline Perms& operator|=(Perms& lhs, Perms rhs)
{
    lhs = lhs | rhs;
    return lhs;
}

inline Perms& operator&=(Perms& lhs, Perms rhs)
{
    lhs = lhs & rhs;
    return lhs;
}

inline bool has_perms(Perms haystack, Perms needle)
{
    return (haystack & needle) == needle;
}
Perms parsePerms(char r, char w, char x, char p);
Perms parsePerms(const std::string_view& p);

struct MemoryRegionProperties
{
    const Perms       perms;
    const std::string regionName;
    const uintptr_t   parentRegionStart; // Inclusive
    const uintptr_t
        parentRegionSize; // Exclusive (start + end - 1 is the last)
    uintptr_t
        relativeRegionStart; // Relative to the parent region start
    uintptr_t   relativeRegionSize; // Relative to parent region size

    const pid_t pid;

    inline uintptr_t getActualRegionStart() const
    {
        return parentRegionStart + relativeRegionStart;
    }
};

class RegionPropertiesList
    : private std::vector<MemoryRegionProperties>
{
  public:
    struct Builder
    {
        std::vector<MemoryRegionProperties> data;

        RegionPropertiesList build() {
            return RegionPropertiesList(std::move(data));
        }
        Builder() = default;
    };

    struct Consolidator
    {
        std::vector<Builder> builders;

        RegionPropertiesList consolidate() 
        {
            // for (auto &builder : builders ) {
            // } 
        }
    };

    using std::vector<MemoryRegionProperties>::size;
    using std::vector<MemoryRegionProperties>::push_back;
    using std::vector<MemoryRegionProperties>::clear;
    using std::vector<MemoryRegionProperties>::reserve;
    using std::vector<MemoryRegionProperties>::resize;
    using std::vector<MemoryRegionProperties>::begin;
    using std::vector<MemoryRegionProperties>::end;
    using std::vector<MemoryRegionProperties>::front;
    using std::vector<MemoryRegionProperties>::back;
    using std::vector<MemoryRegionProperties>::at;
    using std::vector<MemoryRegionProperties>::operator[];

    // Inclusive
    RegionPropertiesList
    filterRegionsByMaxSize(const uintptr_t& maxSize);
    // Inclusive
    RegionPropertiesList
    filterRegionsByMinSize(const uintptr_t& minSize);

    // By name. If you want unnamed regions, use the
    // subname feature with input "UnnamedRegion-".
    RegionPropertiesList
    filterRegionsByName(const std::string_view& name);

    // Checks if the subname is contained in the name.
    RegionPropertiesList
    filterRegionsBySubName(const std::string_view& subName);

    // Must match exactly.
    // IE "rxp", "p", "rwp", "rws"
    RegionPropertiesList
    filterRegionsByPerms(const std::string_view& perms);

    // Does not do it in place.
    RegionPropertiesList
    sortRegionsBySize(const bool increasing = true);

    RegionPropertiesList() = default;
    RegionPropertiesList(
        std::vector<MemoryRegionProperties>&& other) :
        std::vector<MemoryRegionProperties>(other)
    {
    }
};

// Reads the map of a certain pid.
// For unnamed regions, they are named "UnnamedRegion-x", where
// the number of unnamed regions at that point.
RegionPropertiesList readMapsFromPid(pid_t pid);
std::ostream&        operator<<(std::ostream&                 s,
                         const MemoryRegionProperties& m);

#endif // maps_hpp_INCLUDED
