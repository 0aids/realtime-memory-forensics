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
#define register _structRegister

namespace rmf::graph
{
    // Temporary, will deal with those ter
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
    // Literally only for arguments?
    struct Field
    {
        std::string name;
        std::string type;
    };

    struct Struct
    {
        std::string        name;
        std::vector<Field> fields;

        std::vector<Field>::const_iterator
        getFieldIter(std::string_view name) const
        {
            auto beg = fields.cbegin();
            auto end = fields.cend();
            for (; beg != end; beg++)
            {
                if (beg->name == name)
                    break;
            }
            return beg;
        }
    };

    class StructBuilder
    {
      private:
        Struct m_struct;

      public:
        StructBuilder(const std::string_view str) :
            m_struct(std::string(str))
        {
        }
        StructBuilder& field(const std::string_view name,
                             const std::string_view type)
        {
            m_struct.fields.emplace_back(std::string(name),
                                         std::string(type));
            return *this;
        }
        Struct build()
        {
            return m_struct;
        }
    };
    struct StructAlignmentRules
    {
        uint8_t alignedAs;
        size_t  totalSize;
    };

    // Very rudimentary prototype. I can't be bothered to think up of a good
    // DS for this.
    // Does not support square bracket arrays rn because can't be bothered.
    class StructRegistry
    {
      private:
        struct RegisteredStruct
        {
            Struct _struct;
            // Also contains details about offsets for each.
            std::vector<ptrdiff_t> cumulativeOffsets;
            StructAlignmentRules   alignmentRules;
            std::optional<size_t>
            getFieldOffset(std::string_view name) const
            {
                auto fieldIter = _struct.getFieldIter(name);
                if (fieldIter == _struct.fields.cend())
                    return std::nullopt;
                return cumulativeOffsets[fieldIter -
                                         _struct.fields.cbegin()];
            }
        };
        // Probably should change it to
        std::unordered_map<StructTypeId, RegisteredStruct>
            m_registeredStructs;
        std::unordered_map<std::string, StructTypeId> m_nameToId;
        StructTypeId                                  m_idGiver = 1;

      public:
        static constexpr StructTypeId BAD_ID = 0;
        StructRegistry();
        StructRegistry(const StructRegistry&)            = default;
        StructRegistry(StructRegistry&&)                 = default;
        StructRegistry& operator=(const StructRegistry&) = default;
        StructRegistry& operator=(StructRegistry&&)      = default;

        // keyword "register" hidden via #define. who tf even uses this keyword.
        StructTypeId register(const Struct& _struct);
        std::optional<StructTypeId>
        getIdFromString(const std::string_view view);

        std::optional<size_t> getStructSize(StructTypeId id) const;
        std::optional<size_t>
             getFieldOffset(StructTypeId           id,
                            const std::string_view fieldName) const;

        bool containsStructMember(StructMemberId id);
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
