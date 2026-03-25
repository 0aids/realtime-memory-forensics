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
        for (auto& [mrp, keyRefPair] : m_nodeSearchCache)
        {
            keyRefPair.second = m_nodes[keyRefPair.first];
        }

        for (auto& [mrp, keyRefPair] : m_linkSearchCache)
        {
            keyRefPair.second = m_links[keyRefPair.first];
        }
        m_traversalCacheInvalidated = false;
    }

    NodeKey MemoryGraphData::addNode(const MemoryNodeData& data)
    {
        invalidateCache();
        if (containsMrp(data.mrp))
            // Return the original key.
            return m_nodeSearchCache.at(data.mrp).first;
        auto key = m_nodes.insert({data});
        m_nodeSearchCache.emplace(data.mrp,
                                  NodePair{key, m_nodes[key]});
        return key;
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
        if (containsLink(data))
            return m_linkSearchCache.at(data).first;
        LinkKey key = m_links.insert({data, source, target});
        m_linkSearchCache.emplace(data, LinkPair{key, m_links[key]});
        return key;
    }
    bool MemoryGraphData::removeNode(NodeKey key)
    {
        invalidateCache();
        if (!m_nodes.contains(key))
            return false;
        m_nodeSearchCache.erase(m_nodes[key].nodeData.mrp);
        m_nodes.erase(key);
        return true;
    }

    bool MemoryGraphData::removeLink(LinkKey key)
    {
        invalidateCache();
        if (!m_links.contains(key))
            return false;
        m_linkSearchCache.erase(m_links[key].data);
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
        m_nodeSearchCache.erase(m_nodes[key].nodeData.mrp);
        auto newKey = m_nodes.replace(key, {newData});
        m_nodeSearchCache.emplace(newData.mrp,
                                  NodePair{newKey, m_nodes[newKey]});
        return newKey;
    }
    std::optional<LinkKey>
    MemoryGraphData::updateLinkData(LinkKey               key,
                                    const MemoryLinkData& newData)
    {
        if (!m_links.contains(key))
            return std::nullopt;
        MemoryLink oldData = m_links.at(key);
        m_linkSearchCache.erase(oldData.data);
        auto newKey = m_links.replace(
            key, {newData, oldData.sourceNode, oldData.targetNode});
        m_linkSearchCache.emplace(newData,
                                  LinkPair{newKey, m_links[newKey]});
        return newKey;
    }

    // Clears all nodes and links
    void MemoryGraphData::clear()
    {
        m_links.clear();
        m_nodes.clear();
        m_sourceAddrToLink.clear();
        m_targetAddrToLink.clear();
        m_linkSearchCache.clear();
        m_nodeSearchCache.clear();
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
    bool MemoryGraphData::containsMrp(
        const types::MemoryRegionProperties& mrp)
    {
        return m_nodeSearchCache.contains(mrp);
    }

    bool MemoryGraphData::containsLink(const MemoryLinkData& link)
    {
        return m_linkSearchCache.contains(link);
    }
    std::optional<NodeKey> MemoryGraphData::getNodeKeyAtMrp(
        const types::MemoryRegionProperties& mrp)
    {
        if (m_nodeSearchCache.contains(mrp))
            return m_nodeSearchCache.at(mrp).first;
        return std::nullopt;
    }
    std::optional<LinkKey>
    MemoryGraphData::getLinkKeyAtLinkData(const MemoryLinkData& link)
    {
        if (m_linkSearchCache.contains(link))
            return m_linkSearchCache.at(link).first;
        return std::nullopt;
    }

    size_t MemoryGraphData::pruneDeadLinks()
    {
        std::vector<LinkKey> linksToPrune;
        for (const auto& [key, link] : m_links)
        {
            if (!m_nodes.contains(link.sourceNode) ||
                !m_nodes.contains(link.targetNode))
            {
                linksToPrune.push_back(key);
            }
        }
        for (const auto& key : linksToPrune)
        {
            removeLink(key);
        }
        return linksToPrune.size();
    }

    size_t MemoryGraphData::pruneDeadNodes()
    {
        std::vector<NodeKey> nodesToPrune;
        for (const auto& [key, node] : m_nodes)
        {
            if (getChildren(key).empty() && getParents(key).empty())
                nodesToPrune.push_back(key);
        }
        for (const auto& key : nodesToPrune)
        {
            removeNode(key);
        }
        return nodesToPrune.size();
    }
}
