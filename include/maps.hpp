#ifndef maps_hpp_INCLUDED
#define maps_hpp_INCLUDED

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
    const Perms       perms = Perms::EMPTY;
    const std::string regionName = "";
    const uintptr_t   parentRegionStart = 0; // Inclusive
    const uintptr_t
        parentRegionSize = 0; // Exclusive (start + end - 1 is the last)
    uintptr_t
        relativeRegionStart = 0; // Relative to the parent region start
    uintptr_t   relativeRegionSize = 0; // Relative to parent region size

    const pid_t pid = 0;

    inline uintptr_t getActualRegionStart() const
    {
        return parentRegionStart + relativeRegionStart;
    }

    auto operator<=>(const MemoryRegionProperties&) const = default;

    std::string toStr() const;
};

class RegionPropertiesList
    : private std::vector<MemoryRegionProperties>
{
public:
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

    // Must not have the permission.
    // IS if the perm is 'S' to be ignored, will
    // let 'rwp', 'rxp' in, but not 'rs' etc.
    // Probably only going to be used remove shared perms.
    RegionPropertiesList
    filterRegionsByNotPerms(const std::string_view& perms);

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

// Reads the pagemap of the region given to eliminate / split the region
// up into regions that are actually in memory at the moment of scanning.
RegionPropertiesList getActiveRegions(const MemoryRegionProperties &mrp);

RegionPropertiesList getActiveRegionsFromRegionPropertiesList(
    const RegionPropertiesList &rpl
);

RegionPropertiesList breakIntoRegionChunks(
    const RegionPropertiesList &rpl, size_t overlap
);

#endif // maps_hpp_INCLUDED
