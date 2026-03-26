#ifndef memory_graph_hpp_INCLUDED
#define memory_graph_hpp_INCLUDED
#include <cstddef>
#include <functional>
#include <map>
#include <ranges>
#include <string_view>
#include <unordered_map>
#include <optional>
#include <vector>
#include <cstdint>
#include "utils.hpp"
#include "types.hpp"
#include <stdexcept>
#include <utility>

namespace rmf::graph
{
    using StructTypeId = uint64_t;
    struct StructMemberId
    {
        StructTypeId type;
        uint32_t     index;
    };
    using NodeKey = utils::SlotKey;
    using LinkKey = utils::SlotKey;

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
        friend bool    operator==(const MemoryLinkData& a,
                               const MemoryLinkData& b)
        {
            return a.sourceAddr == b.sourceAddr &&
                a.targetAddr == b.targetAddr;
        }
    };
}
template <>
struct std::hash<rmf::graph::StructMemberId>
{
    size_t operator()(const rmf::graph::StructMemberId& ld) const
    {
        return std::hash<rmf::graph::StructTypeId>()(ld.type) ^
            std::hash<uint32_t>()(ld.index);
    }
};

template <>
struct std::hash<rmf::graph::MemoryLinkData>
{
    size_t operator()(const rmf::graph::MemoryLinkData& ld) const
    {
        return std::hash<rmf::graph::StructMemberId>()(
                   ld.sourceMemberId) ^
            std::hash<rmf::graph::StructMemberId>()(
                   ld.targetMemberId) ^
            std::hash<uintptr_t>()(ld.sourceAddr) ^
            std::hash<uintptr_t>()(ld.targetAddr);
    }
};

namespace rmf::graph
{
    struct StringHash
    {
        using is_transparent =
            void; // Marks the hasher as transparent

        std::size_t operator()(std::string_view sv) const
        {
            return std::hash<std::string_view>{}(sv);
        }
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

    struct StructAlignmentRules
    {
        size_t alignedAs;
        size_t totalSize;
    };

    class StructRegistry
    {
      private:
        // Not true data orientated design but i'm lazy.
        // Plus we are not constantly modifying this.
        struct FieldData
        {
            std::string          name;
            std::string          type;
            size_t               cumulativeOffset;
            StructAlignmentRules alignmentRules;
        };
        struct StructData
        {
            std::string            name;
            StructTypeId           id;
            StructAlignmentRules   alignmentRules;
            std::vector<FieldData> fields;
        };

      public:
        // A class that can only exist as a temporary object to
        // assist with cleaner registering.
        class StructBuilder
        {
          public:
            friend class StructRegistry;

          private:
            // Only initialisable by the registry.
            StructRegistry& m_registry;
            StructBuilder(const std::string_view name,
                          StructRegistry&        registry);
            StructData m_data;
            size_t     m_currentOffset = 0;

          public:
            StructBuilder()                                = delete;
            StructBuilder(const StructBuilder&)            = delete;
            StructBuilder(StructBuilder&&)                 = delete;
            StructBuilder& operator=(const StructBuilder&) = delete;
            StructBuilder& operator=(StructBuilder&&)      = delete;
            // Calculates offsets, alignment etc.
            StructBuilder&& field(const std::string_view name,
                                  const std::string_view type);
            StructTypeId    end();
        };

      private:
        StructTypeId m_idGiver = 1;
        std::unordered_map<std::string, StructTypeId, StringHash,
                           std::equal_to<>>
                                                     m_nameToId;
        std::unordered_map<StructTypeId, StructData> m_data;
        StructTypeId _registerStruct(StructData&& data);

      public:
        static constexpr StructTypeId BAD_ID = 0;
        StructRegistry();
        StructRegistry(const StructRegistry&)            = default;
        StructRegistry(StructRegistry&&)                 = default;
        StructRegistry& operator=(const StructRegistry&) = default;
        StructRegistry& operator=(StructRegistry&&)      = default;

        bool            containsFieldId(StructMemberId id) const;
        bool            containsParentId(StructTypeId id) const;
        std::optional<StructTypeId>
        getParentId(const std::string_view name) const;
        std::optional<ptrdiff_t>
        getFieldOffset(StructMemberId id) const;
        std::optional<ptrdiff_t>
        getFieldAlignment(StructMemberId id) const;
        std::optional<StructAlignmentRules>
        getStructAlignmentRules(StructTypeId id) const;
        std::optional<StructAlignmentRules>
        getStructAlignmentRules(const std::string_view view) const;
        std::optional<StructTypeId>
        getParentOfField(StructMemberId id) const;

        std::optional<std::vector<StructMemberId>>
        getFieldsOfParent(StructTypeId id) const;

        // Assuming that the true address is the element at root, reconstructs
        // the mrp around the root to contain the struct.
        types::MemoryRegionProperties restructureMrp(
            StructMemberId                       root,
            const types::MemoryRegionProperties& mrp) const;

        // Actually returns the struct type id, after you call builder.end().
        // otherwise it doesn't.
        StructBuilder registerr(const std::string_view name);
    };

    class MemoryGraphData
    {
      private:
        using NodePair =
            std::pair<utils::SlotKey,
                      std::reference_wrapper<MemoryNode>>;
        using LinkPair =
            std::pair<utils::SlotKey,
                      std::reference_wrapper<MemoryLink>>;
        utils::SlotMap<MemoryNode> m_nodes;
        utils::SlotMap<MemoryLink> m_links;

        // Do not use the reference wrappers unless the cache has been revalidated.
        std::unordered_map<types::MemoryRegionProperties, NodePair>
            m_nodeSearchCache;
        std::unordered_map<MemoryLinkData, LinkPair>
            m_linkSearchCache;

        // Cached stuff for traversal
        bool m_traversalCacheInvalidated = true;

        // Sorted arrays, sorted by node's TrueAddress.
        std::vector<AddrToLink> m_sourceAddrToLink;
        std::vector<AddrToLink> m_targetAddrToLink;

        // Rebuilds the m_sourceAddrToLink and m_targetAddrToLink arrays.
        // Sorts them by address for binary searching (std::lower_bound/std::equal_range).
        void buildTraversalCacheIfNeeded();

        // Marks the cache as dirty. To be called inside addNode, removeNode, etc.
        void invalidateCache();

      public:
        StructRegistry structRegistry;

        MemoryGraphData()                                  = default;
        MemoryGraphData(const MemoryGraphData&)            = default;
        MemoryGraphData(MemoryGraphData&&)                 = default;
        MemoryGraphData& operator=(const MemoryGraphData&) = default;
        MemoryGraphData& operator=(MemoryGraphData&&)      = default;

        ~MemoryGraphData() = default;
        // Default : Adds a node with no links whatsoever
        // If the node already exists, returns a key to that node.
        NodeKey addNode(const MemoryNodeData& data);

        // Moves the root of the mrp to be the field, if given, and then
        // resizes the mrp accordingly.
        // If the structure doesn't exist, or th field is invalid, then we
        // will fail to insert the node.
        std::optional<NodeKey> addStructuredNode(
            const types::MemoryRegionProperties&  mrp,
            const std::string_view                structName,
            const std::optional<std::string_view> field =
                std::nullopt);

        // Default: Adds a link between two nodes.
        std::optional<LinkKey> addLink(NodeKey source, NodeKey target,
                                       const MemoryLinkData& data);

        // I will definitely be asking myself later during testing on
        // how tf to get struct member ids. This is a problem for future me.
        std::optional<LinkKey>
             addLinkStructured(NodeKey source, NodeKey target,
                               StructMemberId sourceMember,
                               StructMemberId targetMember,
                               uintptr_t sourceAddr, uintptr_t targetAddr);
        bool removeNode(NodeKey key);
        bool removeLink(LinkKey key);

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

        bool containsMrp(const types::MemoryRegionProperties& mrp);
        bool containsLink(const MemoryLinkData& link);
        std::optional<NodeKey>
        getNodeKeyAtMrp(const types::MemoryRegionProperties& mrp);
        std::optional<LinkKey>
        getLinkKeyAtLinkData(const MemoryLinkData& link);
        // Returns number of dead links.
        size_t pruneDeadLinks();
        size_t pruneDeadNodes();
    };
}

#endif // memory_graph_hpp_INCLUDED
