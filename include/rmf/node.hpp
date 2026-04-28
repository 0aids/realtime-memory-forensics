#pragma once
#include "rmf/logging/logging.hpp"
#include <string>
#include <type_traits>

namespace RealtimeMemoryForensics
{
    template <typename T>
    concept IsNode = requires(T t) { T::IsNode; };
    template <typename Node_t, typename... Features>
    concept NodeWithFeatures = requires {
        IsNode<Node_t>;
        (std::is_base_of_v<Features, Node_t>, ...);
    };

    template <typename... Args>
    class Node : public Args...
    {
      public:
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

namespace RealtimeMemoryForensics
{
    template <typename... Args>
    Node<Args...>::operator std::string() const
    { return (... + std::string(static_cast<Args>(*this))); }

    // Ensure that we're not being stupid.
    template <typename... Args>
    void Node<Args...>::wellFormed()
    { static_assert(!std::is_polymorphic_v<SelfType>); }

    // IDK why but these conversions are working without me defining them?
    // My guess is that the compiler can use this templated conversions
    // to use the move and copy features of non-templated data structs.

    // TODO: Copying and moving doesn't do anything right now...
    // Need to fix by only instantiating mixins that are present
    // in both.
    // TODO: Implement for copying!!!
    template <typename... Args>
    template <typename TargetFeature, typename OtherNode>
    TargetFeature Node<Args...>::copy(const OtherNode& other)
    {
        using CleanOtherNode = std::decay_t<OtherNode>;
        if constexpr (std::is_base_of_v<TargetFeature,
                                        CleanOtherNode>)
        {
            // rmf_Info("Compatible base copy construction: \n\t{} => "
            //          "{}\n\tBase: {}",
            //          typeid(CleanOtherNode).name(),
            //          typeid(TargetFeature).name(),
            //          typeid(Node<Args...>).name());
            return static_cast<const TargetFeature&>(other);
        }
        else
        {
            // rmf_Info("Incompatible base copy construction: \n\t{} => "
            //          "{}\n\tBase: {}",
            //          typeid(CleanOtherNode).name(),
            //          typeid(TargetFeature).name(),
            //          typeid(Node<Args...>).name());
            return TargetFeature();
        }
    }
    template <typename... Args>
    template <typename TargetFeature, typename OtherNode>
    TargetFeature Node<Args...>::move(OtherNode&& other)
    {
        using CleanOtherNode = std::decay_t<OtherNode>;
        if constexpr (std::is_base_of_v<TargetFeature,
                                        CleanOtherNode>)
        {
            rmf_Info("Compatible base move construction: \n\t{} => "
                     "{}\n\tBase: {}",
                     typeid(CleanOtherNode).name(),
                     typeid(TargetFeature).name(),
                     typeid(Node<Args...>).name());
            return std::move(static_cast<TargetFeature&>(other));
        }
        else
        {
            rmf_Info("Incompatible base move construction: \n\t{} => "
                     "{}\n\tBase: {}",
                     typeid(CleanOtherNode).name(),
                     typeid(TargetFeature).name(),
                     typeid(Node<Args...>).name());
            return TargetFeature();
        }
    }
    template <typename... Args>
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
    template <typename... OtherArgs>
    Node<Args...>::Node(const Node<OtherArgs...>& other) :
        Args(Node<Args...>::copy<Args>(other))...
    {
    }
    template <typename... Args>
    template <typename... OtherArgs>
    Node<Args...>&
    Node<Args...>::operator=(Node<OtherArgs...>&& other)
    { return std::move(static_cast<Node<Args...>>(other)); }
    template <typename... Args>
    template <typename... OtherArgs>
    Node<Args...>&
    Node<Args...>::operator=(const Node<OtherArgs...>& other)
    { return static_cast<Node<Args...>>(other); }
}
