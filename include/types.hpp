#ifndef types_hpp_INCLUDED
#define types_hpp_INCLUDED

#include <concepts>
#include <optional>
#include <vector>
#include <cstdint>
#include <string>
#include <format>
#include <magic_enum/magic_enum_all.hpp>
#include <magic_enum/magic_enum_flags.hpp>

// Really janky shit to get magic enum working for this flag?
namespace rmf::types {
enum class Perms:uint8_t;
};

enum class rmf::types::Perms:uint8_t{
    None = 0,
    Read = 1,
    Write = 2,
    Execute = 4,
    Shared = 8, // Defaults to private.
    // Unimplemented, but may consider implementing.
    // MayRead = 16,
    // MayWrite = 32,
    // MayExecute = 64,
};

template <>
struct magic_enum::customize::enum_range<rmf::types::Perms> {
  static constexpr bool is_flags = true;
};


namespace rmf::types {
    template <typename T>
    concept Numeral = std::integral<T> || std::floating_point<T>;

inline bool hasPerms(Perms haystack, Perms needle)
{
    using namespace magic_enum::bitwise_operators; // out-of-the-box bitwise operators for enums.
    return (haystack & needle) == needle;
}

struct MemoryRegionProperties {
    uintptr_t parentRegionAddress;
    uintptr_t parentRegionSize;
    uintptr_t relativeRegionAddress;
    uintptr_t relativeRegionSize;
    std::string regionName;
    Perms perms;
    pid_t pid;

    inline uintptr_t TrueAddress() const
    {
        return parentRegionAddress + relativeRegionAddress;
    }
    inline uintptr_t TrueEnd() const
    {
        return parentRegionAddress + relativeRegionAddress + relativeRegionSize;
    }

    inline uintptr_t relativeEnd() const {
        return relativeRegionAddress + relativeRegionSize;
    }

    // Method below is written by ai.
    std::string toString() const {
        std::string displayName = regionName;
        constexpr size_t visibleLength = 35;

        #define d_extension "[.]"
        if (regionName.length() > visibleLength) {
            displayName = d_extension + regionName.substr(regionName.length() - visibleLength - (sizeof(d_extension) - 1));
        }
        #undef d_extension

        return std::format(
            "[{}] Addr: 0x{:x} (Size: 0x{:x}) | Rel: 0x{:x} (Size: 0x{:x}) | Perms: {}",
            displayName,
            parentRegionAddress,
            parentRegionSize,
            relativeRegionAddress,
            relativeRegionSize,
            magic_enum::enum_flags_name(perms) // Handles bitmask names automatically
        );
    }

};

using SnapshotDataBuffer = std::vector<uint8_t>;

// DO NOT CONSTRUCT MANUALLY, call MemorySnapshot::Make static method!
// This is for compatibility with threadpools, without having to create lambdas.
class MemorySnapshot {
private:
    SnapshotDataBuffer mc_data;
    MemorySnapshot(const MemoryRegionProperties &_mrp);

public:
    const MemoryRegionProperties mrp;
    std::span<const uint8_t> getData() const {
        return mc_data; // implicit into std::span
    }
    static MemorySnapshot Make(const MemoryRegionProperties &mrp);

    inline bool isValid() const {
        return mc_data.size() == mrp.relativeRegionSize;
    }

    void printHex(size_t charsPerLine=32, size_t numLines=SIZE_MAX) const;
};

class RefreshableSnapshot {
private:
    std::optional<MemorySnapshot> m_snapshot;
public:
    MemoryRegionProperties mrpModdable;
    void refresh();
    void refresh(const MemoryRegionProperties& mrp);
};

using MemoryRegionPropertiesVec = std::vector<rmf::types::MemoryRegionProperties>;
};

#endif // types_hpp_INCLUDED
