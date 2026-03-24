#ifndef memory_graph_hpp_INCLUDED
#define memory_graph_hpp_INCLUDED
#include "types.hpp"
#include "utils.hpp"
#include <functional>
#include <map>
#include <ranges>
#include <unordered_map>
#include <optional>
#include <vector>
#include <cstdint>
#include <stdexcept>

namespace rmf::graph
{
    // Temporary, will deal with those ter
    using StructTypeId   = uint64_t;
    using StructMemberId = uint64_t;
    using NodeKey        = utils::SlotKey;
    using LinkKey        = utils::SlotKey;

    struct MemoryNodeData
    {
        types::MemoryRegionProperties mrp;
        StructTypeId                  structTypeId;
    };
    struct MemoryNode
    {
        MemoryNodeData nodeData;
    };

    struct MemoryLinkData
    {
        StructMemberId sourceMemberId;
        StructMemberId targetMemberId;
        uintptr_t      sourceAddr;
        uintptr_t      targetAddr;
    };

    struct MemoryLink
    {
        MemoryLinkData data;
        NodeKey        sourceNode;
        NodeKey        targetNode;
    };
    struct AddrToLink
    {
        // Address of the node could be source or target depending on which it is.
        uintptr_t addr;
        LinkKey   key;
        bool      operator<(const AddrToLink& other)
        {
            return addr < other.addr;
        }
        bool operator<(const uintptr_t& other)
        {
            return addr < other;
        }
        friend bool operator<(const uintptr_t& a, const AddrToLink& b)
        {
            return a < b.addr;
        }
    };

    class MemoryGraphData
    {
      public:
      private:
        utils::SlotMap<MemoryNode> m_nodes;
        utils::SlotMap<MemoryLink> m_links;
        using SortedNodePair =
            std::pair<utils::SlotKey,
                      std::reference_wrapper<MemoryNode>>;

        // Cached stuff for traversal
        bool m_traversalCacheInvalidated = true;

        // Sorted arrays, sorted by node's TrueAddress.
        std::vector<AddrToLink>     m_sourceAddrToLink;
        std::vector<AddrToLink>     m_targetAddrToLink;
        std::vector<SortedNodePair> m_sortedNodes;

        static bool sortNodePair(const SortedNodePair& a,
                                 const SortedNodePair& b)
        {
            return a.second.get().nodeData.mrp.TrueAddress() <
                b.second.get().nodeData.mrp.TrueAddress();
        };
        static bool sortNodePairLowerBound(const SortedNodePair& a,
                                           uintptr_t             addr)
        {
            return a.second.get().nodeData.mrp.TrueAddress() < addr;
        }

        static bool sortNodePairUpperBound(uintptr_t             addr,
                                           const SortedNodePair& a)
        {
            return a.second.get().nodeData.mrp.TrueAddress() > addr;
        }
        // Rebuilds the m_sourceAddrToLink and m_targetAddrToLink arrays.
        // Sorts them by address for binary searching (std::lower_bound/std::equal_range).
        void buildTraversalCacheIfNeeded();

        // Marks the cache as dirty. To be called inside addNode, removeNode, etc.
        void invalidateCache();

        std::vector<SortedNodePair>::const_iterator
        getSortedLowerBound(uintptr_t addr)
        {
            buildTraversalCacheIfNeeded();
            auto begin = std::lower_bound(m_sortedNodes.begin(),
                                          m_sortedNodes.end(), addr,
                                          sortNodePairLowerBound);
            return begin;
        }

        std::vector<SortedNodePair>::const_iterator
        getSortedUpperBound(uintptr_t addr)
        {
            buildTraversalCacheIfNeeded();
            auto begin = std::upper_bound(m_sortedNodes.begin(),
                                          m_sortedNodes.end(), addr,
                                          sortNodePairUpperBound);
            return begin;
        }

      public:
        MemoryGraphData()                                  = default;
        MemoryGraphData(const MemoryGraphData&)            = default;
        MemoryGraphData(MemoryGraphData&&)                 = default;
        MemoryGraphData& operator=(const MemoryGraphData&) = default;
        MemoryGraphData& operator=(MemoryGraphData&&)      = default;

        ~MemoryGraphData() = default;
        // Default : Adds a node with no links whatsoever
        NodeKey addNode(const MemoryNodeData& data);

        // Default: Adds a link between two nodes.
        std::optional<LinkKey> addLink(NodeKey source, NodeKey target,
                                       const MemoryLinkData& data);
        bool                   removeNode(NodeKey key);
        bool                   removeLink(LinkKey key);

        // Getters for Node
        std::optional<MemoryNode> getNode(NodeKey key);

        // Getters for Link
        std::optional<MemoryLink> getLink(LinkKey key);

        // Optional: Methods to update underlying data without replacing the node
        // This will delete and create the node inplace, invalidating the links
        // connected to it.
        std::optional<NodeKey>
        updateNodeData(NodeKey key, const MemoryNodeData& newData);
        // Does not invalidate it?
        std::optional<LinkKey>
        updateLinkData(LinkKey key, const MemoryLinkData& newData);
        // Retrieve adjacent links for a specific node
        auto getOutgoingLinks(NodeKey nodeKey)
        {
            buildTraversalCacheIfNeeded();
            auto trueAddr =
                m_nodes.at(nodeKey).nodeData.mrp.TrueAddress();
            auto lowerBound =
                std::lower_bound(m_sourceAddrToLink.begin(),
                                 m_sourceAddrToLink.end(), trueAddr);
            auto upperBound =
                std::upper_bound(m_sourceAddrToLink.begin(),
                                 m_sourceAddrToLink.end(), trueAddr);
            return std::ranges::subrange(lowerBound, upperBound) |
                std::views::transform(
                       [](auto&& p) -> decltype(auto)
                       {
                           return (p.key); // or p.key
                       });
        }

        auto getIncomingLinks(NodeKey nodeKey)
        {
            buildTraversalCacheIfNeeded();
            auto trueAddr =
                m_nodes.at(nodeKey).nodeData.mrp.TrueAddress();
            auto lowerBound =
                std::lower_bound(m_targetAddrToLink.begin(),
                                 m_targetAddrToLink.end(), trueAddr);
            auto upperBound =
                std::upper_bound(m_targetAddrToLink.begin(),
                                 m_targetAddrToLink.end(), trueAddr);
            return std::ranges::subrange(lowerBound, upperBound) |
                std::views::transform(
                       [](auto&& p) -> decltype(auto)
                       {
                           return (p.key); // or p.key
                       });
        }

        // Helper to get connected nodes directly
        auto getChildren(NodeKey parentKey)
        {
            buildTraversalCacheIfNeeded();
            auto outgoingLinks = getOutgoingLinks(parentKey);
            return outgoingLinks |
                std::views::transform(
                       [this](auto&& p) -> decltype(auto)
                       {
                           return (m_nodes.at(p)); // or p.key
                       });
        }

        auto getParents(NodeKey childKey)
        {
            buildTraversalCacheIfNeeded();
            auto incomingLinks = getIncomingLinks(childKey);
            return incomingLinks |
                std::views::transform(
                       [this](auto&& p) -> decltype(auto)
                       {
                           return (m_nodes.at(p)); // or p.key
                       });
        }
        // Clears all nodes and links
        void clear();

        // Capacity and sizing
        size_t getNodeCount() const;
        size_t getLinkCount() const;
        bool   isEmpty() const;

        // Checks if a key is currently valid (hasn't been removed)
        bool isValidNode(NodeKey key) const;
        bool isValidLink(LinkKey key) const;

        const utils::SlotMap<MemoryNode>& getNodes() const;
        const utils::SlotMap<MemoryLink>& getLinks() const;

        std::optional<NodeKey> getNodeKeyAtAddr(uintptr_t addr);
        std::optional<MemoryNodeData> getNodeAtAddr(uintptr_t addr);
        std::optional<NodeKey>
        getNodeKeyContainingAddr(uintptr_t addr);
        std::optional<MemoryNodeData>
        getNodeContainingAddr(uintptr_t addr);
    };
}

#endif // memory_graph_hpp_INCLUDED
