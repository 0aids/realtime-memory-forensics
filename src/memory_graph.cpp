#include "memory_graph.hpp"
#include "logger.hpp"
#include "types.hpp"
#include "utils.hpp"
#include <algorithm>
#include <optional>
#include <ranges>

namespace rmf::graph
{
    void MemoryGraphData::invalidateCache()
    {
        m_traversalCacheInvalidated = true;
    }
    void MemoryGraphData::buildTraversalCacheIfNeeded()
    {
        if (!m_traversalCacheInvalidated)
            return;
        // Refresh the cache
        // TODO: Incrementally add stuff to the cache.
        m_sourceAddrToLink.clear();
        m_targetAddrToLink.clear();
        // m_sortedNodes = m_nodes |
        //     std::ranges::to<std::vector<
        //         std::pair<utils::SlotKey,
        //                   std::reference_wrapper<MemoryNode>>>>();

        // std::sort(m_sortedNodes.begin(), m_sortedNodes.end(),
        //           sortNodePair);

        for (const auto& [key, node] : m_links)
        {
            m_sourceAddrToLink.push_back(
                {m_nodes.at(node.sourceNode)
                     .nodeData.mrp.TrueAddress(),
                 key});
            m_targetAddrToLink.push_back(
                {m_nodes.at(node.targetNode)
                     .nodeData.mrp.TrueAddress(),
                 key});
        }
        std::sort(m_sourceAddrToLink.begin(),
                  m_sourceAddrToLink.end(),
                  [](const AddrToLink& a, const AddrToLink& b)
                  { return a.addr < b.addr; });
        std::sort(m_targetAddrToLink.begin(),
                  m_targetAddrToLink.end(),
                  [](const AddrToLink& a, const AddrToLink& b)
                  { return a.addr < b.addr; });
        m_traversalCacheInvalidated = false;
    }

    NodeKey MemoryGraphData::addNode(const MemoryNodeData& data)
    {
        invalidateCache();
        return m_nodes.insert({data});
    }

    std::optional<LinkKey>
    MemoryGraphData::addLink(NodeKey source, NodeKey target,
                             const MemoryLinkData& data)
    {
        invalidateCache();
        if (!m_nodes.contains(source) || !m_nodes.contains(target))
        {
            return std::nullopt;
        }
        LinkKey key = m_links.insert({data, source, target});
        return key;
    }
    bool MemoryGraphData::removeNode(NodeKey key)
    {
        invalidateCache();
        if (!m_nodes.contains(key))
            return false;
        m_nodes.erase(key);
        return true;
    }

    bool MemoryGraphData::removeLink(LinkKey key)
    {
        invalidateCache();
        if (!m_links.contains(key))
            return false;
        m_links.erase(key);
        return true;
    }

    // Getters for Node
    std::optional<MemoryNode> MemoryGraphData::getNode(NodeKey key)
    {
        if (!m_nodes.contains(key))
            return std::nullopt;
        return m_nodes.at(key);
    }

    // Getters for Link
    std::optional<MemoryLink> MemoryGraphData::getLink(LinkKey key)
    {
        if (!m_links.contains(key))
            return std::nullopt;
        return m_links.at(key);
    }

    // Optional: Methods to update underlying data without replacing the node
    // Invalidates all links to the node.
    std::optional<NodeKey>
    MemoryGraphData::updateNodeData(NodeKey               key,
                                    const MemoryNodeData& newData)
    {
        if (!m_nodes.contains(key))
            return std::nullopt;
        return m_nodes.replace(key, {newData});
    }
    std::optional<LinkKey>
    MemoryGraphData::updateLinkData(LinkKey               key,
                                    const MemoryLinkData& newData)
    {
        if (!m_links.contains(key))
            return std::nullopt;
        MemoryLink oldData = m_links.at(key);
        return m_links.replace(
            key, {newData, oldData.sourceNode, oldData.targetNode});
    }
    // Retrieve adjacent links for a specific node
    auto MemoryGraphData::getOutgoingLinks(NodeKey nodeKey)
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

    auto MemoryGraphData::getIncomingLinks(NodeKey nodeKey)
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
    auto MemoryGraphData::getChildren(NodeKey parentKey)
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

    auto MemoryGraphData::getParents(NodeKey childKey)
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
    void MemoryGraphData::clear()
    {
        m_links.clear();
        m_nodes.clear();
        m_sourceAddrToLink.clear();
        m_targetAddrToLink.clear();
    }

    // Capacity and sizing
    size_t MemoryGraphData::getNodeCount() const
    {
        return m_nodes.size();
    }
    size_t MemoryGraphData::getLinkCount() const
    {
        return m_links.size();
    }
    bool MemoryGraphData::isEmpty() const
    {
        return m_nodes.empty();
    }

    // Checks if a key is currently MemoryGraphData::valid (hasn't been removed)
    bool MemoryGraphData::isValidNode(NodeKey key) const
    {
        return m_nodes.contains(key);
    }
    bool MemoryGraphData::isValidLink(LinkKey key) const
    {
        return m_links.contains(key);
    }

    // Only const iterators.

    const utils::SlotMap<MemoryNode>&
    MemoryGraphData::getNodes() const
    {
        return m_nodes;
    }
    const utils::SlotMap<MemoryLink>&
    MemoryGraphData::getLinks() const
    {
        return m_links;
    }

    std::optional<NodeKey>
    MemoryGraphData::getNodeKeyContainingAddr(uintptr_t addr)
    {
        for (const auto& [key, node] : m_nodes)
        {
            if (node.nodeData.mrp.TrueAddress() <= addr &&
                addr < node.nodeData.mrp.TrueEnd())
                return key;
        }
        return std::nullopt;
    }

    std::optional<MemoryNodeData>
    MemoryGraphData::getNodeContainingAddr(uintptr_t addr)
    {
        for (const auto& [key, node] : m_nodes)
        {
            if (node.nodeData.mrp.TrueAddress() <= addr &&
                addr < node.nodeData.mrp.TrueEnd())
                return node.nodeData;
        }
        return std::nullopt;
    }

    std::optional<NodeKey>
    MemoryGraphData::getNodeKeyAtAddr(uintptr_t addr)
    {
        for (const auto& [key, node] : m_nodes)
        {
            if (node.nodeData.mrp.TrueAddress() == addr)
                return key;
        }
        return std::nullopt;
    }

    std::optional<MemoryNodeData>
    MemoryGraphData::getNodeAtAddr(uintptr_t addr)
    {
        for (const auto& [key, node] : m_nodes)
        {
            if (node.nodeData.mrp.TrueAddress() == addr)
                return node.nodeData;
        }
        return std::nullopt;
    }
}
