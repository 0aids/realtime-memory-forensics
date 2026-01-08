// AI was used to quickly add the example colornode from the imnodes
// examples.
#ifndef RMF_GUI_EXAMPLES_HPP
#define RMF_GUI_EXAMPLES_HPP

#include <vector>
#include <algorithm>
#include <stack>
#include <string>
#include <optional>
#include <cassert>

#include <imgui.h>
#include <imnodes.h>

namespace rmf::gui::examples
{

    template <typename ElementType>
    struct Span
    {
        using iterator = ElementType*;
        template <typename Container>
        Span(Container& c) : begin_(c.data()), end_(begin_ + c.size())
        {
        }
        iterator begin() const
        {
            return begin_;
        }
        iterator end() const
        {
            return end_;
        }

      private:
        iterator begin_;
        iterator end_;
    };

    template <typename ElementType>
    class IdMap
    {
      public:
        using iterator = typename std::vector<ElementType>::iterator;
        using const_iterator =
            typename std::vector<ElementType>::const_iterator;

        const_iterator begin() const
        {
            return elements_.begin();
        }
        const_iterator end() const
        {
            return elements_.end();
        }
        Span<const ElementType> elements() const
        {
            return elements_;
        }
        bool empty() const
        {
            return sorted_ids_.empty();
        }
        size_t size() const
        {
            return sorted_ids_.size();
        }

        std::pair<iterator, bool> insert(int                id,
                                         const ElementType& element);
        std::pair<iterator, bool> insert(int           id,
                                         ElementType&& element);
        size_t                    erase(int id);
        void                      clear();
        iterator                  find(int id);
        const_iterator            find(int id) const;
        bool                      contains(int id) const;

      private:
        std::vector<ElementType> elements_;
        std::vector<int>         sorted_ids_;
    };

    // --- IdMap Template Implementations ---
    template <typename ElementType>
    std::pair<typename IdMap<ElementType>::iterator, bool>
    IdMap<ElementType>::insert(const int          id,
                               const ElementType& element)
    {
        auto lower_bound = std::lower_bound(sorted_ids_.begin(),
                                            sorted_ids_.end(), id);
        if (lower_bound != sorted_ids_.end() && id == *lower_bound)
        {
            return std::make_pair(
                std::next(
                    elements_.begin(),
                    std::distance(sorted_ids_.begin(), lower_bound)),
                false);
        }
        auto insert_element_at = std::next(
            elements_.begin(),
            std::distance(sorted_ids_.begin(), lower_bound));
        sorted_ids_.insert(lower_bound, id);
        return std::make_pair(
            elements_.insert(insert_element_at, element), true);
    }

    template <typename ElementType>
    std::pair<typename IdMap<ElementType>::iterator, bool>
    IdMap<ElementType>::insert(const int id, ElementType&& element)
    {
        auto lower_bound = std::lower_bound(sorted_ids_.begin(),
                                            sorted_ids_.end(), id);
        if (lower_bound != sorted_ids_.end() && id == *lower_bound)
        {
            return std::make_pair(
                std::next(
                    elements_.begin(),
                    std::distance(sorted_ids_.begin(), lower_bound)),
                false);
        }
        auto insert_element_at = std::next(
            elements_.begin(),
            std::distance(sorted_ids_.begin(), lower_bound));
        sorted_ids_.insert(lower_bound, id);
        return std::make_pair(
            elements_.insert(insert_element_at, std::move(element)),
            true);
    }

    template <typename ElementType>
    size_t IdMap<ElementType>::erase(const int id)
    {
        auto lower_bound = std::lower_bound(sorted_ids_.begin(),
                                            sorted_ids_.end(), id);
        if (lower_bound == sorted_ids_.end() || id != *lower_bound)
            return 0ull;
        auto erase_element_at = std::next(
            elements_.begin(),
            std::distance(sorted_ids_.begin(), lower_bound));
        sorted_ids_.erase(lower_bound);
        elements_.erase(erase_element_at);
        return 1ull;
    }

    template <typename ElementType>
    void IdMap<ElementType>::clear()
    {
        elements_.clear();
        sorted_ids_.clear();
    }

    template <typename ElementType>
    typename IdMap<ElementType>::iterator
    IdMap<ElementType>::find(const int id)
    {
        const auto lower_bound = std::lower_bound(
            sorted_ids_.cbegin(), sorted_ids_.cend(), id);
        return (lower_bound == sorted_ids_.cend() ||
                *lower_bound != id) ?
            elements_.end() :
            std::next(
                elements_.begin(),
                std::distance(sorted_ids_.cbegin(), lower_bound));
    }

    template <typename ElementType>
    typename IdMap<ElementType>::const_iterator
    IdMap<ElementType>::find(const int id) const
    {
        const auto lower_bound = std::lower_bound(
            sorted_ids_.cbegin(), sorted_ids_.cend(), id);
        return (lower_bound == sorted_ids_.cend() ||
                *lower_bound != id) ?
            elements_.cend() :
            std::next(
                elements_.cbegin(),
                std::distance(sorted_ids_.cbegin(), lower_bound));
    }

    template <typename ElementType>
    bool IdMap<ElementType>::contains(const int id) const
    {
        const auto lower_bound = std::lower_bound(
            sorted_ids_.cbegin(), sorted_ids_.cend(), id);
        return lower_bound != sorted_ids_.cend() &&
            *lower_bound == id;
    }

    template <typename NodeType>
    class Graph
    {
      public:
        struct Edge
        {
            int id, from, to;
            Edge() = default;
            Edge(const int id, const int f, const int t) :
                id(id), from(f), to(t)
            {
            }
            inline int opposite(const int n) const
            {
                return n == from ? to : from;
            }
            inline bool contains(const int n) const
            {
                return n == from || n == to;
            }
        };

        Graph() : current_id_(0) {}
        NodeType&        node(int node_id);
        const NodeType&  node(int node_id) const;
        Span<const int>  neighbors(int node_id) const;
        Span<const Edge> edges() const;
        size_t           num_edges_from_node(int node_id) const;
        int              insert_node(const NodeType& node);
        void             erase_node(int node_id);
        int              insert_edge(int from, int to);
        void             erase_edge(int edge_id);

      private:
        int                     current_id_;
        IdMap<NodeType>         nodes_;
        IdMap<int>              edges_from_node_;
        IdMap<std::vector<int>> node_neighbors_;
        IdMap<Edge>             edges_;
    };

    // --- Graph Template Implementations ---
    template <typename NodeType>
    NodeType& Graph<NodeType>::node(const int id)
    {
        return const_cast<NodeType&>(
            static_cast<const Graph*>(this)->node(id));
    }

    template <typename NodeType>
    const NodeType& Graph<NodeType>::node(const int id) const
    {
        auto iter = nodes_.find(id);
        assert(iter != nodes_.end());
        return *iter;
    }

    template <typename NodeType>
    Span<const int> Graph<NodeType>::neighbors(int node_id) const
    {
        auto iter = node_neighbors_.find(node_id);
        assert(iter != node_neighbors_.end());
        return *iter;
    }

    template <typename NodeType>
    Span<const typename Graph<NodeType>::Edge>
    Graph<NodeType>::edges() const
    {
        return edges_.elements();
    }

    template <typename NodeType>
    size_t Graph<NodeType>::num_edges_from_node(const int id) const
    {
        auto iter = edges_from_node_.find(id);
        assert(iter != edges_from_node_.end());
        return *iter;
    }

    template <typename NodeType>
    int Graph<NodeType>::insert_node(const NodeType& node)
    {
        const int id = current_id_++;
        nodes_.insert(id, node);
        edges_from_node_.insert(id, 0);
        node_neighbors_.insert(id, std::vector<int>());
        return id;
    }

    template <typename NodeType>
    void Graph<NodeType>::erase_node(const int id)
    {
        static std::vector<int> edges_to_erase;
        for (const auto& edge : edges_.elements())
        {
            if (edge.contains(id))
                edges_to_erase.push_back(edge.id);
        }
        for (const int edge_id : edges_to_erase)
            erase_edge(edge_id);
        edges_to_erase.clear();
        nodes_.erase(id);
        edges_from_node_.erase(id);
        node_neighbors_.erase(id);
    }

    template <typename NodeType>
    int Graph<NodeType>::insert_edge(const int from, const int to)
    {
        const int id = current_id_++;
        edges_.insert(id, Edge(id, from, to));
        *edges_from_node_.find(from) += 1;
        node_neighbors_.find(from)->push_back(to);
        return id;
    }

    template <typename NodeType>
    void Graph<NodeType>::erase_edge(const int edge_id)
    {
        const Edge& edge       = *edges_.find(edge_id);
        int&        edge_count = *edges_from_node_.find(edge.from);
        edge_count -= 1;
        auto neighbors = node_neighbors_.find(edge.from);
        auto iter =
            std::find(neighbors->begin(), neighbors->end(), edge.to);
        neighbors->erase(iter);
        edges_.erase(edge_id);
    }

    template <typename NodeType, typename Visitor>
    void dfs_traverse(const Graph<NodeType>& graph,
                      const int start_node, Visitor visitor)
    {
        std::stack<int> stack;
        stack.push(start_node);
        while (!stack.empty())
        {
            const int current_node = stack.top();
            stack.pop();
            visitor(current_node);
            for (const int neighbor : graph.neighbors(current_node))
                stack.push(neighbor);
        }
    }

    enum class NodeType
    {
        add,
        multiply,
        output,
        sine,
        time,
        value
    };

    struct Node
    {
        NodeType type;
        float    value;
        explicit Node(const NodeType t) : type(t), value(0.f) {}
        Node(const NodeType t, const float v) : type(t), value(v) {}
    };

    template <class T>
    T clamp(T x, T a, T b)
    {
        return std::min(b, std::max(x, a));
    }

    ImU32 evaluate(const Graph<Node>& graph, const int root_node);

    class ColorNodeEditor
    {
      public:
        ColorNodeEditor();
        void show(bool* is_open = NULL);

      private:
        enum class UiNodeType
        {
            add,
            multiply,
            output,
            sine,
            time
        };
        struct UiNode
        {
            UiNodeType type;
            int        id;
            union
            {
                struct
                {
                    int lhs, rhs;
                } add;
                struct
                {
                    int lhs, rhs;
                } multiply;
                struct
                {
                    int r, g, b;
                } output;
                struct
                {
                    int input;
                } sine;
            } ui;
        };

        Graph<Node>            graph_;
        std::vector<UiNode>    nodes_;
        int                    root_node_id_;
        ImNodesMiniMapLocation minimap_location_;
    };

} // namespace rmf::gui::examples

#endif
