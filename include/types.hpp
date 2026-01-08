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
        pid_t            pid   = 0;

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
        MemoryRegionPropertiesVec
        BreakIntoChunks(uintptr_t chunkSize,
                        uintptr_t overlapSize = 0);
    };

    using SnapshotDataBuffer = std::vector<uint8_t>;

    // DO NOT CONSTRUCT MANUALLY, call MemorySnapshot::Make static method!
    // This is for compatibility with threadpools, without having to create lambdas.
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
        std::span<const uint8_t> getDataSpan() const
        {
            return d->mc_data; // implicit into std::span
        }

        std::vector<uint8_t> getData() const
        {
            return d->mc_data;
        }

        const MemoryRegionProperties& getMrp() const
        {
            return d->mrp;
        }

        static MemorySnapshot Make(const MemoryRegionProperties& mrp);

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
        FilterPerms(const std::string_view& perms);
        rmf::types::MemoryRegionPropertiesVec
        FilterHasPerms(const std::string_view& perms);
        rmf::types::MemoryRegionPropertiesVec
        FilterNotPerms(const std::string_view& perms);
        rmf::types::MemoryRegionPropertiesVec
        BreakIntoChunks(uintptr_t chunkSize,
                        uintptr_t overlapSize = 0);
    };
    using MemoryRegionID             = uint64_t;
    static constexpr uintptr_t NO_ID = UINTPTR_MAX;
    using MemoryLinkID               = uint64_t;
    class MemoryGraph;
    enum class MemoryLinkPolicy
    {
        Strict, // Will only create the links if both ends are memory regions.
        CreateSource, // Will create the
        CreateTarget,
        CreateSourceTarget,
    };
    struct MemoryRegionLinkData
    {
        MemoryGraph*       parentGraph = nullptr;
        MemoryLinkID       id          = NO_ID;
        MemoryLinkPolicy   policy      = MemoryLinkPolicy::Strict;

        uintptr_t          sourceAddr = 0;
        uintptr_t          targetAddr = 0;
        std::string        name       = "";
        inline std::string toString() const
        {
            return std::format("id: {} policy: {} SourceAddress: "
                               "{:#x} TargetAddress: {:#x} name: {}",
                               id, magic_enum::enum_name(policy),
                               sourceAddr, targetAddr, name);
        }
    };
    struct MemoryRegionLink : public MemoryRegionLinkData
    {
        // struct Hash {
        //     MemoryLinkID operator()(const MemoryRegionLink& link) const noexcept {
        //         return link.id;
        //     }
        // };
    };
    struct MemoryRegionData
    {
        struct NamedValue
        {
            std::string name;
            ptrdiff_t   offset;
            std::string comment;
        };
        MemoryRegionProperties mrp;
        // For now just use a vector to store, and perform O(n)
        // Everytime we search. this is because Im lazy and I want the
        // name to be mutable.
        MemoryGraph*            parentGraph = nullptr;
        MemoryRegionID          id          = NO_ID;
        std::vector<NamedValue> namedValues{};
        std::string             name    = "";
        std::string             comment = "";
        inline std::string      toString() const
        {
            std::string result = std::format(
                "name: {} id: {}\ncomment: {}", name, id, comment);

            for (const auto& namedValue : namedValues)
            {
                result +=
                    std::format("\n\tNamed value: {} offset: "
                                "{:#x}\n\tcomment: {}",
                                namedValue.name, namedValue.offset,
                                namedValue.comment);
            }

            return result;
        }
    };

    // Inherit for ease of use.
    // Not actually making use of oop inheritance, just that the
    // data will also be used for transactions.
    struct MemoryRegion : public MemoryRegionData
    {
      public:
        // This should be generated using the graph builder methods
        std::unordered_set<MemoryLinkID> outgoingLinkIDs{};
        std::unordered_set<MemoryLinkID> incomingLinkIDs{};
        RefreshableSnapshot              rsnap{};

        void AddNamedValue(const std::string& string,
                           ptrdiff_t          offset,
                           const std::string& comment);
        void RemoveNamedValue(const std::string& strin);
        void RefreshSnapshot();
        // The one stored in the refreshable snapshot
        MemoryRegionProperties GetSnapshotMrp();
        std::string      toStringDetailed(bool showLinkDetails=false) const;
    };

    namespace transaction
    {
        // Info to create nodes, update nodes, creating links, etc.
        // Using std::variant + std::visit
        struct CreateMemoryRegion : public MemoryRegionData
        {
        };
        // Updates it by reassigning all these values?
        // Under the hood we'll also ensure that in the creation
        // to this call we return this struct but filled out, and
        // then the user can freely change what they want.
        // Afterwards it will be move assigned back.
        struct UpdateMemoryRegion : public MemoryRegionData
        {
        };
        struct DeleteMemoryRegion
        {
            MemoryRegionID id;
        };
        struct CreateMemoryLink : public MemoryRegionLinkData
        {
        };
        struct UpdateMemoryLink : public MemoryRegionLinkData
        {
        };
        struct DeleteMemoryLink
        {
            MemoryLinkID id;
        };
        using TransactionType =
            std::variant<CreateMemoryRegion, UpdateMemoryRegion,
                         DeleteMemoryRegion, CreateMemoryLink,
                         UpdateMemoryLink, DeleteMemoryLink>;
    }

    class MemoryGraph
    {
        static constexpr uintptr_t NO_ID = UINTPTR_MAX;

      public:
        struct RegionSpan
        {
            MemoryRegionID id;
            uintptr_t      start;
            uintptr_t      end;
        };

      private:
        // An ID of 0 is invalid.
        MemoryRegionID m_counterMemoryRegionID = 1;
        MemoryLinkID   m_counterMemoryLinkID   = 1;
        MemoryRegionPropertiesVec m_parentRegions;

        // TODO: A sorted vector containing locations of memory regions.
        // Supports intersecting/nested regions.
        // Sorts by true address start.
        // std::vector<RegionSpan> m_memoryRegionSpatialIndex;

        MemoryRegionData
        _GetRegionDataFromName(const std::string& name);
        MemoryRegionLinkData
        _GetLinkDataFromName(const std::string& name);

        // void _CreateInverseTransaction(transaction::TransactionType t);
        void _CreateMemoryRegion(transaction::CreateMemoryRegion t);
        void _UpdateMemoryRegion(transaction::UpdateMemoryRegion t);
        void _DeleteMemoryRegion(transaction::DeleteMemoryRegion t);
        void _CreateMemoryLink(transaction::CreateMemoryLink t);
        void _UpdateMemoryLink(transaction::UpdateMemoryLink t);
        void _DeleteMemoryLink(transaction::DeleteMemoryLink t);

        void _ResolveLink();

      public:
        // Used to autoresolve new regions' parent regions.
        // This is not a user friendly way of giving values...
        // For now we'll just hard loop this.
        // TODO: Add string to ID aliases.
        std::unordered_map<MemoryLinkID, MemoryRegionLink>
            m_memoryLinkMap;
        std::unordered_map<MemoryRegionID, MemoryRegion>
            m_memoryRegionMap;

        // Adding the region, strips it of any found links as they will be regenerated later.
        // If you want to add links, add them to via AddMemoryLink
        // void AddMemoryRegion(const MemoryRegion& region);
        // void RemoveMemoryRegion(MemoryRegionID id);

        // void AddMemoryLink(const MemoryRegionLink& link);
        // void RemoveMemoryLink(MemoryLinkID id);
        void Clear();

        bool
        ProcessTransaction(transaction::TransactionType transaction);

        transaction::CreateMemoryRegion
        StartCreateMemoryRegionTransaction();
        transaction::UpdateMemoryRegion
        StartUpdateMemoryRegionTransaction(
            std::optional<std::string>    name = std::nullopt,
            std::optional<MemoryRegionID> id   = std::nullopt);
        transaction::DeleteMemoryRegion
        StartDeleteMemoryRegionTransaction(
            std::optional<std::string>    name = std::nullopt,
            std::optional<MemoryRegionID> id   = std::nullopt);

        transaction::CreateMemoryLink
        StartCreateMemoryLinkTransaction();
        transaction::UpdateMemoryLink
        StartUpdateMemoryLinkTransaction(
            std::optional<std::string>  name = std::nullopt,
            std::optional<MemoryLinkID> id   = std::nullopt);
        transaction::DeleteMemoryLink
        StartDeleteMemoryLinkTransaction(
            std::optional<std::string>  name = std::nullopt,
            std::optional<MemoryLinkID> id   = std::nullopt);

        MemoryRegionID
               FindMemoryRegionContainingAddress(uintptr_t addr);

        size_t GetMemoryRegionMapSize() const;
        size_t GetMemoryRegionLinkMapSize() const;

        const MemoryRegion&     GetMemoryRegionFromID(MemoryRegionID id) const;
        const MemoryRegionLink& GetMemoryRegionLinkFromID(MemoryLinkID id) const;

        MemoryGraph() = default;
    };
};

#endif // types_hpp_INCLUDED
