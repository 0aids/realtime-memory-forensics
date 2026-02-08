#ifndef memory_graph_hpp_INCLUDED
#define memory_graph_hpp_INCLUDED
#include "types.hpp"
#include <ranges>
#include <unordered_map>
#include <optional>

namespace rmf::graph
{
    // Possible policies for construction of memory links
    enum class MemoryLinkPolicy : uint8_t
    {
        Strict, // Will only create the links if both ends are memory regions.
        CreateSource,
        CreateTarget,
        CreateSourceTarget,
    };
    using MemoryRegionID               = uint64_t;
    static constexpr uintptr_t noID_ce = 0;
    using MemoryLinkID                 = uint64_t;
    class MemoryGraph;


    // All relevant data of a memory region link
    struct MemoryLinkData
    {
        MemoryLinkPolicy policy      = MemoryLinkPolicy::Strict;

        uintptr_t        sourceAddr = 0;
        uintptr_t        targetAddr = 0;
        std::string      name       = "";
        MemoryRegionID   sourceID   = noID_ce;
        MemoryRegionID   targetID   = noID_ce;
    };

    // The actual link itself. (edge in the graph).
    // Do not create manually!!!
    struct MemoryLink
    {
        MemoryLinkData data;
        MemoryGraph*     parentGraph = nullptr;
        MemoryLinkID     id          = noID_ce;
        std::string      toString() const;
    };

    // All relevant data of a memory region for use in node graphs.
    struct MemoryRegionData
    {
        struct NamedValue
        {
            std::string name;
            types::type type;
            ptrdiff_t   offset;
            std::string comment;
        };
        rmf::types::MemoryRegionProperties mrp;
        std::vector<NamedValue>            namedValues{};
        std::string                        name    = "";
        std::string                        comment = "";
    };

    // Do not create manually!!!
    struct MemoryRegion
    {
        MemoryRegionData data;
        MemoryGraph*                       parentGraph = nullptr;
        MemoryRegionID                     id          = noID_ce;
        std::string                        toString() const;
    };

	// Desired workflow:
	// 		Use in tandem with the analyzer to receive memory regions.
    class MemoryGraph
    {
        struct MemoryRegionRange {
            MemoryRegionID id;
            uintptr_t startAddr;
            uintptr_t endAddr;
        };

        MemoryLinkID m_nextValidLinkID   = 1;
        MemoryLinkID m_nextValidRegionID = 1;
        std::unordered_map<MemoryLinkID, MemoryLink>     m_links{};
        std::unordered_map<MemoryRegionID, MemoryRegion> m_regions{};
        // TODO: If it becomes too slow with a large amount of regions,
        // Make use of this + MemoryRegionRanges to perform binary
        // search.
        std::vector<MemoryRegionRange> range;

        MemoryRegionID _LinkCreateOrGetSource(uintptr_t sourceAddr);
        MemoryRegionID _LinkCreateOrGetTarget(uintptr_t targetAddr);

      public:
        // Makes a copy, but it's basically trivially copyable.
        MemoryRegionID RegionAdd(MemoryRegionData mr);
        void           RegionsRefreshAll();
        void           RegionDelete(MemoryRegionID id);
        // Returns nullopt if id doesn't exist.
        std::optional<MemoryRegion*>
             RegionGetFromID(MemoryRegionID id);
        auto RegionsGetViews()
        {
            return std::views::values(m_regions);
        }
        MemoryRegionID RegionGetRegionIdAtAddress(uintptr_t addr);
        std::optional<MemoryRegion*>
                     RegionGetRegionAtAddress(uintptr_t addr);
        MemoryRegionID RegionGetRegionIdContainingAddress(uintptr_t addr);
        std::optional<MemoryRegion*>
                     RegionGetRegionContainingAddress(uintptr_t addr);

        MemoryLinkID LinkNaiveAdd(MemoryLinkData data);
        void         LinkDelete(MemoryLinkID id);
        auto         LinksGetViews()
        {
            return std::views::values(m_links);
        }
        std::optional<MemoryLink*> LinkGetFromID(MemoryLinkID id);
        // Will also generate nodes according to the policy.
        MemoryLinkID LinkSmartAdd(MemoryLinkData data);

        std::vector<MemoryLinkID>
        UpdateLinks(MemoryLinkPolicy policy);

		// Assigns floating mrps to their respective parent regions + offsets.
        void RegionsAssignToMaps(const types::MemoryRegionPropertiesVec &OGMaps);

        // Dump all mrps in here.
        void RegionsAddMrpVec(const types::MemoryRegionPropertiesVec &mrpVec, std::string name = "");
    };
}

#endif // memory_graph_hpp_INCLUDED
