#pragma once
#include "rmf/logging/logging.hpp"
#include "rmf/utils/meta.hpp"
#include "rmf/utils/expect.hpp"
#include <string>
#include <type_traits>

namespace rmf
{
    template <typename... Args>
    concept NodeRequirements = requires {
        NodeExclusions::isExclusive<Args...>() &&
            !Meta::HasDuplicates_v<Args...>;
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

    template <typename T>
    struct EmptyFeature
    {
    };

    template <typename T, typename... Args>
    using NodeAddFeature_t =
        typename std::conditional_t<Meta::HasType<T, Args...>::value, T,
                                    EmptyFeature<T>>;

    // Consider forcing a sort of strict order using enable if
    // like
    template <typename... Args>
        requires NodeRequirements<Args...>
    class Node : public Utils::Error,
                 public NodeAddFeature_t<Map, Args...>,
                 public NodeAddFeature_t<Snapshot, Args...>,
                 public NodeAddFeature_t<Typed, Args...>,
                 public NodeAddFeature_t<Struct, Args...>,
                 public NodeAddFeature_t<Pointer, Args...>,
                 public NodeAddFeature_t<Field, Args...>,
                 public NodeAddFeature_t<Primitive, Args...>,
                 public NodeAddFeature_t<Array, Args...>
    {
      public:
        // Stupid coupling but I couldn't figure it out how to not duplicate it.
        using Features = std::tuple<
            NodeAddFeature_t<Map, Args...>, NodeAddFeature_t<Snapshot, Args...>,
            NodeAddFeature_t<Typed, Args...>, NodeAddFeature_t<Struct, Args...>,
            NodeAddFeature_t<Pointer, Args...>,
            NodeAddFeature_t<Field, Args...>,
            NodeAddFeature_t<Primitive, Args...>,
            NodeAddFeature_t<Array, Args...>>;
        // Add typed, field, primitive, array, struct etc to the node.
        // Will swap mutually exclusive types to ensure it works properly.
        template <IsType T>
        using WithType =
            Node<std::conditional_t<NodeExclusions::isExclusive<Args, T>(),
                                    Args, EmptyFeature<Args>>...,
                 T>;

        // Ensures that we have the feature specified. If it already exists, does nothing
        // Otherwises adds it.
        template <IsFeature T>
        using WithFeature =
            Node<Args..., std::conditional_t<!Meta::HasType<T, Args...>::value,
                                             T, EmptyFeature<T>>>;

        // Removes the feature if it exists. Can remove type or feature.
        template <typename ToRemove>
        using WithoutFeature =
            Node<std::conditional_t<!std::same_as<Args, ToRemove>, Args,
                                    EmptyFeature<Args>>...>;

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
        Node(const Args&...);
        // Construct a new node with an extra feature.
        template <typename Feature>
        WithFeature<Feature> addFeature(const Feature& f) const;

        template <typename... OtherArgs>
        Node(Node<OtherArgs...>&&);
        template <typename... OtherArgs>
        Node(const Node<OtherArgs...>&);
        template <typename... OtherArgs>
        Node& operator=(Node<OtherArgs...>&&);
        template <typename... OtherArgs>
        Node& operator=(const Node<OtherArgs...>&);
              operator std::string() const;
        using DerivedType = Node<Args...>;
        using SelfType    = Node;

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
        static_assert(!Meta::HasDuplicates_v<Args...>);
    }

    template <typename... Args>
        requires NodeRequirements<Args...>
    Node<Args...>::Node(const Args&... args) : Args(args)...
    {
    }

    template <typename... Args>
        requires NodeRequirements<Args...>
    template <typename Feature>
    Node<Args...>::WithFeature<Feature>
    Node<Args...>::addFeature(const Feature& f) const
    {
        // This should now call the Node(Args...) constructor?
        return Node<Args...>::WithFeature<Feature>(static_cast<Args>(*this)...,
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
    // BUG: for types which are convertible from each other, they are not directly the base,
    // so we should check if we can convert any of them to our a base. IE Struct is convertible into Typed,
    // etc, but this is not deduced by this move or copy constructor helper.
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
