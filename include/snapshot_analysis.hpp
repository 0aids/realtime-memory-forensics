#ifndef snapshot_analysis_hpp_INCLUDED
#define snapshot_analysis_hpp_INCLUDED

#include "maps.hpp"
#include "snapshots.hpp"

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


RegionPropertiesList findStrRegions(const MemorySnapshot &snap, const std::string& str);

// Must be the absolute value of the pointer;
// finds all pointers which point to the given memory address.
RegionPropertiesList findPtrRegions(const MemorySnapshot &snap, const uintptr_t& ptr);

RegionPropertiesList findChangedRegions(const MemorySnapshot &snap1, const MemorySnapshot &snap2, size_t numBytes);

RegionPropertiesList findUnchangedRegions(const MemorySnapshot &snap1, const MemorySnapshot &snap2, size_t numBytes);

// Temporary ellipsis operator.
// TODO: Implement so it searches for a pointer to a specific region/s.
RegionPropertiesList findPtrlikeRegions(const MemorySnapshot &snap, ...);

RegionPropertiesList findStrlikeRegions(const MemorySnapshot &snap);

RegionPropertiesList findRegionsStruct(
    const std::vector<StructProperties>& structProperties_l);

#endif // snapshot_analysis_hpp_INCLUDED
