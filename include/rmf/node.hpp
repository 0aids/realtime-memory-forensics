#pragma once
#include "rmf/logging/logging.hpp"
#include "rmf/utils/meta.hpp"
#include <string>
#include <type_traits>

namespace rmf
{
    template <typename... Args>
    concept NodeRequirements = requires {
        NodeExclusions::isExclusive<Args...>() &&
            !Utils::Meta::HasDuplicates_v<Args...>;
    };

    template <typename... Args>
        requires NodeRequirements<Args...>
    class Node;

    template <typename T>
    concept IsNode = requires(T t) { std::decay_t<T>::IsNode; };

    template <typename Node_t, typename... Features>
    concept NodeWithFeatures = requires {
        IsNode<Node_t>;
        (std::is_base_of_v<Features, Node_t>, ...);
    };

    // Consider forcing a sort of strict order using enable if
    // like
    template <typename... Args>
        requires NodeRequirements<Args...>
    class Node : public Args...
    {
      public:
        template <typename T>
        using AddFeature = Node<Args..., T>;

        // Removes the feature if it exists
        template <typename ToRemove>
        using WithoutFeature =
            Node<std::enable_if<!std::same_as<Args, ToRemove>, Args>...>;

        template <typename ToRemove, typename ToAdd>
        using SwapFeature = Node<
            std::conditional<std::same_as<Args, ToRemove>, ToAdd, Args>...>;

        // I feel like this sort of stuff is becoming illegal.
        template <typename ToRemove, typename ToAdd>
        using SwapOrAddFeature =
            AddFeature<ToAdd>::template WithoutFeature<ToRemove>;

        struct VecOp : public Args::VecOp...
        {
        };
        constexpr static inline bool IsNode = true;
        Node()                              = default;
        Node(Node&&)                        = default;
        Node(const Node&)                   = default;
        Node& operator=(Node&&)             = default;
        Node& operator=(const Node&)        = default;

        // For allowing conversions between nodes.
        Node(Args...);
        // Construct a new node with an extra feature.
        template <typename Feature>
        AddFeature<Feature> addFeature(Feature f) const;
        template <typename... OtherArgs>
        Node(Node<OtherArgs...>&&);
        template <typename... OtherArgs>
        Node(const Node<OtherArgs...>&);
        template <typename... OtherArgs>
        Node& operator=(Node<OtherArgs...>&&);
        template <typename... OtherArgs>
        Node& operator=(const Node<OtherArgs...>&);
        using DerivedType = Node<Args...>;
        using SelfType    = Node;
             operator std::string() const;
        void wellFormed();

      private:
        template <typename TargetFeature, typename OtherFeature>
        static TargetFeature copy(const OtherFeature& other);
        template <typename TargetFeature, typename OtherFeature>
        static TargetFeature move(OtherFeature&& other);
    };
}

namespace rmf
{
    template <typename... Args>
        requires NodeRequirements<Args...>
    Node<Args...>::operator std::string() const
    {
        return (... + std::string(static_cast<Args>(*this)));
    }

    // Ensure that we're not being stupid.
    template <typename... Args>
        requires NodeRequirements<Args...>
    void Node<Args...>::wellFormed()
    {
        using namespace std;
        static_assert(!is_polymorphic_v<SelfType>);
        static_assert(!Utils::Meta::HasDuplicates_v<Args...>);
    }

    template <typename... Args>
        requires NodeRequirements<Args...>
    Node<Args...>::Node(Args... args) : Args(args)...
    {
    }

    template <typename... Args>
        requires NodeRequirements<Args...>
    template <typename Feature>
    Node<Args...>::AddFeature<Feature>
    Node<Args...>::addFeature(Feature f) const
    {
        return Node<Args...>::AddFeature<Feature>(static_cast<Args>(*this)...,
                                                  f);
    }

    template <typename... Args>
        requires NodeRequirements<Args...>
    template <typename TargetFeature, typename OtherNode>
    TargetFeature Node<Args...>::copy(const OtherNode& other)
    {
        using CleanOtherNode = std::decay_t<OtherNode>;
        if constexpr (std::is_base_of_v<TargetFeature, CleanOtherNode>)
        {
            return static_cast<const TargetFeature&>(other);
        }
        else
        {
            return TargetFeature();
        }
    }
    template <typename... Args>
        requires NodeRequirements<Args...>
    template <typename TargetFeature, typename OtherNode>
    TargetFeature Node<Args...>::move(OtherNode&& other)
    {
        using CleanOtherNode = std::decay_t<OtherNode>;
        if constexpr (std::is_base_of_v<TargetFeature, CleanOtherNode>)
        {
            return std::move(static_cast<TargetFeature&>(other));
        }
        else
        {
            return TargetFeature();
        }
    }
    template <typename... Args>
        requires NodeRequirements<Args...>
    template <typename... OtherArgs>
    Node<Args...>::Node(Node<OtherArgs...>&& other) :
        Args(Node<Args...>::move<Args>(other))...
    {
        /* Possible solution for mixins that rely on other mixins.
         * Just have to agree on the so called "stages" of initialisation.
         * ( [&]() {
         *   static_cast<Args&>(other).after();
         * }(),
         *  ...);
         */
    }
    template <typename... Args>
        requires NodeRequirements<Args...>
    template <typename... OtherArgs>
    Node<Args...>::Node(const Node<OtherArgs...>& other) :
        Args(Node<Args...>::copy<Args>(other))...
    {
    }
    template <typename... Args>
        requires NodeRequirements<Args...>
    template <typename... OtherArgs>
    Node<Args...>& Node<Args...>::operator=(Node<OtherArgs...>&& other)
    {
        return std::move(static_cast<Node<Args...>>(other));
    }
    template <typename... Args>
        requires NodeRequirements<Args...>
    template <typename... OtherArgs>
    Node<Args...>& Node<Args...>::operator=(const Node<OtherArgs...>& other)
    {
        return static_cast<Node<Args...>>(other);
    }
}
