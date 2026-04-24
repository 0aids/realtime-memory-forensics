#pragma once
#include <string>
#include <type_traits>

namespace RealtimeMemoryForensics
{
    template <typename T>
    concept IsNode = requires(T t) { T::IsNode; };
    // Very sus CRTP (curiosly recurring template pattern)
    // I think I bit off more than i could chew...
    template <template <typename> typename... Args>
    class Node : public Args<Node<Args...>>...
    {

      public:
        constexpr static inline bool IsNode = true;
        Node()                              = default;
        Node(Node&&)                        = default;
        Node(const Node&)                   = default;
        Node& operator=(Node&&)             = default;
        Node& operator=(const Node&)        = default;

        // For allowing conversions between nodes.
        template <template <typename> typename... OtherArgs>
        Node(Node<OtherArgs...>&&);
        template <template <typename> typename... OtherArgs>
        Node(const Node<OtherArgs...>&);
        template <template <typename> typename... OtherArgs>
        Node& operator=(Node<OtherArgs...>&&);
        template <template <typename> typename... OtherArgs>
        Node& operator=(const Node<OtherArgs...>&);
        using DerivedType = Node<Args...>;
        using SelfType    = Node;
             operator std::string() const;
        void wellFormed();
    };
}

namespace RealtimeMemoryForensics
{
    template <template <typename> typename... Args>
    Node<Args...>::operator std::string() const
    {
        return (... +
                std::string(static_cast<Args<Node<Args>>>(*this)));
    }

    // Ensure that we're not being stupid.
    template <template <typename> typename... Args>
    void Node<Args...>::wellFormed()
    { static_assert(!std::is_polymorphic_v<SelfType>); }

    // IDK why but these conversions are working without me defining them?
    // My guess is that the compiler can use this templated conversions
    // to use the move and copy features of non-templated data structs.
    template <template <typename> typename... Args>
    template <template <typename> typename... OtherArgs>
    Node<Args...>::Node(Node<OtherArgs...>&&)
    {
    }
    template <template <typename> typename... Args>
    template <template <typename> typename... OtherArgs>
    Node<Args...>::Node(const Node<OtherArgs...>&)
    {
    }
    template <template <typename> typename... Args>
    template <template <typename> typename... OtherArgs>
    Node<Args...>& Node<Args...>::operator=(Node<OtherArgs...>&&)
    {
    }
    template <template <typename> typename... Args>
    template <template <typename> typename... OtherArgs>
    Node<Args...>& Node<Args...>::operator=(const Node<OtherArgs...>&)
    {
    }
}
