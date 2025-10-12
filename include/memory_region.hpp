#pragma once
#include "region_properties.hpp"
#include <functional>
#include <atomic>
#include <thread>
#include <array>
#include <cstdint>
#include <ctime>
#include <memory>
#include <string_view>
#include <vector>
#include <chrono>

// Give the constructor an empty struct of the type you want to find.
// Constructs a list of pointers and their offsets to find in memory.

enum class StructMemberTypes
{
    unknown_t = 0,
    ptr_t     = 1,
    int_t     = 2,
    uint_t    = 3,
    char_t    = 4,
    float_t   = 5,
    // etc, just for casting or smth.
};

struct StructProperties
{
    const std::string_view  name;
    const size_t            offset;
    const size_t            size;
    const StructMemberTypes type;
};

#define d_RegisterMember(StructType, MemberName, typeEnum)           \
    {                                                                \
        #MemberName,                                                 \
        offsetof(StructType::MemberName),                            \
        sizeof(decltype(StructType::MemberName)),                    \
        typeEnum,                                                    \
    }

struct StructProperties_l : public std::vector<StructProperties>
{

};

struct RegionSnapshot : public std::vector<char>
{
  private:
    inline static bool failed = false;

  public:
    std::chrono::nanoseconds snapshottedTime;
    // This is to allow the parent memory region
    // to truncate it's memory reading area without
    // breaking previous regions. When comparisons are done
    // We'll read the actual property that it was snapshotted with
    // And this will ensure that the we don't do stupid shit.
    const MemoryRegionProperties regionProperties;

    std::string                  toStr() const;

    // Compare region for a certain comparison size (in bytes).
    // It returns a list of regionProperties with the corresponding changed regions.
    // Only compares the 2 regions if they have the same properties.
    RegionPropertiesList
    findChangedRegions(const RegionSnapshot& otherRegion,
                       const uint32_t        compareSize) const;

    RegionPropertiesList
    findUnchangedRegions(const RegionSnapshot& otherRegion,
                         const uint32_t        compareSize) const;

    // Find a string-like region - a region that contains alphanumeric stuff.
    RegionPropertiesList
    findStringLikeRegions(const size_t& minLength);
    RegionSnapshot(std::chrono::nanoseconds     time,
                   const MemoryRegionProperties regionProps) :
        snapshottedTime(time), regionProperties(regionProps) {};

    // Find all regions with that string.
    RegionPropertiesList findOf(const std::string& str);

    // Find all pointers which point to a region.
    RegionPropertiesList findPointers(const uintptr_t& actualAddress);

    RegionPropertiesList findPointerLikes();

    // Must be the absolute address.
    // Returns the linked list in the correct order.
    // Only traverses backwards!!!
    RegionPropertiesList
    findLinkedList(const uintptr_t& memberAddress,
                   const uintptr_t& numHeaderBytes);

    // Finds a struct. Only really uses the pointers to find the region.
    RegionPropertiesList
    findStruct(const StructProperties_l& structProperties);

    RegionPropertiesList findDoubleLike(const double& lower,
                                        const double& upper);
};

using SP_RegionSnapshot = std::shared_ptr<RegionSnapshot>;

struct SnapshotList_SP : public std::vector<SP_RegionSnapshot>
{
};

class MemoryRegion
{
  public:
    pid_t                  m_pid;
    MemoryRegionProperties m_regionProperties;
    SnapshotList_SP        m_snapshots_l;
    MemoryRegion(MemoryRegionProperties properties, pid_t pid);
    SP_RegionSnapshot getLastSnapshot() const;
    // True if succeeded.
    bool snapshot();
};

