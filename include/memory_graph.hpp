#ifndef memory_graph_hpp_INCLUDED
#define memory_graph_hpp_INCLUDED
#include "types.hpp"
#include <ranges>
#include <unordered_map>
#include <optional>

namespace rmf::graph
{
    using MemoryRegionID               = uint64_t;
    static constexpr uintptr_t noID_ce = UINTPTR_MAX;
    using MemoryLinkID                 = uint64_t;
    class MemoryGraph;

	// Possible policies for construction of memory links
    enum class MemoryLinkPolicy
    {
        Strict, // Will only create the links if both ends are memory regions.
        CreateSource, 
        CreateTarget,
        CreateSourceTarget,
    };

	// All relevant data of a memory region link
    struct MemoryRegionLinkData
    {
        MemoryGraph*     parentGraph = nullptr;
        MemoryLinkID     id          = noID_ce;
        MemoryLinkPolicy policy = MemoryLinkPolicy::Strict;

        uintptr_t        sourceAddr = 0;
        uintptr_t        targetAddr = 0;
        std::string      name       = "";
        std::string      toString() const;
    };

	// The actual link itself. (edge in the graph).
	// Do not create manually!!!
    struct MemoryRegionLink
    {
        MemoryRegionLinkData data;
    };

	// All relevant data of a memory region for use in node graphs.
    struct MemoryRegionData
    {
        struct NamedValue
        {
            std::string name;
            ptrdiff_t   offset;
            std::string comment;
        };
        rmf::types::MemoryRegionProperties mrp;
        // For now just use a vector to store, and perform O(n)
        // Everytime we search. this is because Im lazy and I want the
        // name to be mutable.
        MemoryGraph*            parentGraph = nullptr;
        MemoryRegionID          id          = noID_ce;
        std::vector<NamedValue> namedValues{};
        std::string             name    = "";
        std::string             comment = "";
        std::string      toString() const;
    };

	// Do not create manually!!!
    struct MemoryRegion {
        MemoryRegionData data;
    };

    class MemoryGraph
    {
        MemoryLinkID m_nextValidLinkID = 0;
        MemoryLinkID m_nextValidRegionID = 0;
        std::unordered_map<MemoryLinkID, MemoryRegionLink> m_links{};
        std::unordered_map<MemoryRegionID, MemoryRegion> m_regions{};

    public:
        // Makes a copy, but it's basically trivially copyable.
        MemoryRegionID AddRegion(MemoryRegionData mr);
        void RegionsRefreshAll();
        void RegionDelete(MemoryRegionID id);
        // Returns nullopt if id doesn't exist.
        std::optional<MemoryRegion*> RegionGetFromID(MemoryRegionID id);
        auto RegionsGetViews() {
            return std::views::values(m_regions);
        }
    };
}

#endif // memory_graph_hpp_INCLUDED
