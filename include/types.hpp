#ifndef types_hpp_INCLUDED
#define types_hpp_INCLUDED

#include "logger.hpp"
#include <sstream>
#include <unordered_set>
#include <cstddef>
#include <memory>
#include <concepts>
#include <optional>
#include <variant>
#include <vector>
#include <cstdint>
#include <string>
#include <format>
#include <unordered_map>
#include <magic_enum/magic_enum.hpp>
#include <magic_enum/magic_enum_flags.hpp>

// Really janky shit to get magic enum working for this flag?
namespace rmf::types
{
    enum class Perms : uint8_t;
};

enum class rmf::types::Perms : uint8_t
{
    None    = 0,
    Read    = 1,
    Write   = 2,
    Execute = 4,
    Shared  = 8, // Defaults to private.
    // Unimplemented, but may consider implementing.
    // MayRead = 16,
    // MayWrite = 32,
    // MayExecute = 64,
};

template <>
struct magic_enum::customize::enum_range<rmf::types::Perms>
{
    static constexpr bool is_flags = true;
};

namespace rmf::types
{

    enum class typeName : uint8_t
    {
        _bool,
        _char,
        _i8,
        _i16,
        _i32,
        _i64,
        _u8,
        _u16,
        _u32,
        _u64,
        _f32,
        _f64,
        _ptr,
    };
    struct type
    {
        typeName type;
    };

    enum class compositeTypeName : uint8_t
    {
        array,
        string,
        unorderedMap,
        orderedMap,
    };

    template <typename T>
    concept Numeral = std::integral<T> || std::floating_point<T>;
    class MemoryRegionPropertiesVec;

    inline bool hasPerms(Perms haystack, Perms needle)
    {
        using namespace magic_enum::
            bitwise_operators; // out-of-the-box bitwise operators for enums.
        return (haystack & needle) == needle;
    }

    struct MemoryRegionProperties
    {
        // Default values for safety
        uintptr_t                          parentRegionAddress   = 0;
        uintptr_t                          parentRegionSize      = 0;
        uintptr_t                          relativeRegionAddress = 0;
        uintptr_t                          relativeRegionSize    = 0;
        std::shared_ptr<const std::string> regionName_sp =
            std::make_shared<const std::string>("");
        Perms            perms = Perms::None;

        inline uintptr_t TrueAddress() const
        {
            return parentRegionAddress + relativeRegionAddress;
        }
        inline uintptr_t TrueEnd() const
        {
            return parentRegionAddress + relativeRegionAddress +
                relativeRegionSize;
        }

        inline uintptr_t relativeEnd() const
        {
            return relativeRegionAddress + relativeRegionSize;
        }

        // Method below is written by ai.
        std::string toString() const
        {
            using namespace std::string_literals;
            std::string      displayName   = *regionName_sp;
            constexpr size_t visibleLength = 35;

            try
            {
                if (regionName_sp->length() > visibleLength)
                {
                    displayName = "[.]"s +
                        regionName_sp->substr(
                            regionName_sp->length() - visibleLength -
                            3);
                }
            }
            catch (const std::exception& e)
            {
                rmf_Log(rmf_Error,
                        "Error when shortening display name...");
                rmf_Log(rmf_Error,
                        "Fault display name: " << *regionName_sp);
                rmf_Log(rmf_Error, "Error is: " << e.what());
                displayName = "Error Occurred";
            }

            return std::format(
                "[{}] Addr: 0x{:x} (Size: 0x{:x}) | Rel: 0x{:x} "
                "(Size: 0x{:x}) | Perms: {}",
                displayName, parentRegionAddress, parentRegionSize,
                relativeRegionAddress, relativeRegionSize,
                magic_enum::enum_flags_name(
                    perms) // Handles bitmask names automatically
            );
        }

        void
        AssignNewParentRegion(const MemoryRegionProperties& other);

        MemoryRegionPropertiesVec
        BreakIntoChunks(uintptr_t chunkSize,
                        uintptr_t overlapSize = 0);
    };

    using SnapshotDataBuffer = std::vector<uint8_t>;

    // DO NOT CONSTRUCT MANUALLY, call MemorySnapshot::Make static method!
    // This is for compatibility with threadpools, without having to create lambdas.
    // Copies are light, implementation hidden with sharedptrs.
    class MemorySnapshot
    {
      private:
        struct Data
        {
            const MemoryRegionProperties mrp;
            SnapshotDataBuffer           mc_data;
            Data(const MemoryRegionProperties _mrp) : mrp(_mrp) {}
        };
        MemorySnapshot(const MemoryRegionProperties& _mrp);
        std::shared_ptr<Data> d;

      public:
        MemorySnapshot() = delete;
        // Generic copy and move stuff tho
        MemorySnapshot(MemorySnapshot&&)                    = default;
        MemorySnapshot(const MemorySnapshot&)               = default;
        MemorySnapshot&    operator=(MemorySnapshot&&)      = default;
        MemorySnapshot&    operator=(const MemorySnapshot&) = default;

        std::span<uint8_t> getDataSpan() const
        {
            return d->mc_data; // implicit into std::span
        }

        // Removed getData as it copies the entire buffer, which is what we don't want.

        const MemoryRegionProperties& getMrp() const
        {
            return d->mrp;
        }

        static MemorySnapshot Make(const MemoryRegionProperties& mrp,
                                   pid_t                         pid);

        inline bool           isValid() const
        {
            return d->mc_data.size() == d->mrp.relativeRegionSize;
        }

        void                        printHex(size_t charsPerLine = 32,
                                             size_t numLines = SIZE_MAX) const;

        std::shared_ptr<const Data> getImpl() const
        {
            return d;
        }
    };

    class RefreshableSnapshot
    {
      private:
        std::optional<MemorySnapshot> m_snapshot;

      public:
        MemoryRegionProperties mrpModdable;
        void                   refresh();
        void refresh(const MemoryRegionProperties& mrp);
    };

    // a class which extends std::vector for filtering functions.
    class MemoryRegionPropertiesVec
        : public std::vector<MemoryRegionProperties>
    {
      public:
        using std::vector<MemoryRegionProperties>::vector;
        rmf::types::MemoryRegionPropertiesVec
        FilterMaxSize(const uintptr_t maxSize);
        rmf::types::MemoryRegionPropertiesVec
        FilterMinSize(const uintptr_t minSize);
        rmf::types::MemoryRegionPropertiesVec
        FilterName(const std::string_view& string);
        rmf::types::MemoryRegionPropertiesVec
        FilterContainsName(const std::string_view& string);
        rmf::types::MemoryRegionPropertiesVec
        FilterExactPerms(const std::string_view& perms);
        rmf::types::MemoryRegionPropertiesVec
        FilterHasPerms(const std::string_view& perms) const;
        rmf::types::MemoryRegionPropertiesVec
        FilterNotPerms(const std::string_view& perms);
        rmf::types::MemoryRegionPropertiesVec
        BreakIntoChunks(uintptr_t chunkSize,
                        uintptr_t overlapSize = 0);
        std::optional<MemoryRegionProperties>
        GetRegionContainingAddress(uintptr_t addr) const;

        rmf::types::MemoryRegionPropertiesVec
        FilterActiveRegions(pid_t pid);

        // Implicit conversion from vector to mrpvec
        MemoryRegionPropertiesVec(
            std::vector<MemoryRegionProperties>&& other);
        MemoryRegionPropertiesVec(
            const std::vector<MemoryRegionProperties>& other);
    };
};

#endif // types_hpp_INCLUDED
