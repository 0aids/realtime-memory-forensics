#pragma once
#include <string>
#include <type_traits>

namespace RealtimeMemoryForensics
{
    // Very sus CRTP (curiosly recurring template pattern)
    template <template <typename> typename... Args>
    class Node : public Args<Node<Args...>>...
    {

      public:
        using DerivedType = Node<Args...>;
        using SelfType    = Node;
        operator std::string() const;
        ~Node();
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
}
