#pragma once
#include "rmf/node.hpp"
#include <algorithm>
#include <functional>
#include <iterator>
#include <ranges>
#include <memory>
#include <type_traits>
#include <vector>
namespace RealtimeMemoryForensics::Utils
{
    // Mixins for providing vectorised operations.
    template <typename T>
    class DefaultOperator
    {
    };

    template <typename T, typename = void>
    struct VecOpTraits
    {
        template <typename DerivedVec>
        using type = DefaultOperator<DerivedVec>;
    };

    template <typename... Features>
    struct VecOpTraits<Node<Features...>>
    {
        template <typename DerivedVec>
        using type = Node<Features...>::template VecOp<DerivedVec>;
    };

    template <typename T,
              template <typename> typename Operator = DefaultOperator,
              typename Allocator = std::allocator<T>>
    class Vec : public std::vector<T, Allocator>,
                public Operator<Vec<T, Operator, Allocator>>
    {
      public:
        using BaseType = std::vector<T, Allocator>;
        using BaseType::BaseType;
        using InnerType = T;

        // TODO: Threaded wrapper.
        template <typename F, typename... Args>
        auto map(F&& f, Args&&... args);

        template <typename F, typename... Args>
            requires std::is_same_v<std::invoke_result_t<F, Args...>,
                                    bool>
        Vec<T> filter(F&& f, Args&&... args);
    };
}

namespace RealtimeMemoryForensics::Utils
{
    template <typename T, template <typename> typename Operator,
              typename Allocator>
    template <typename F, typename... Args>
    auto Vec<T, Operator, Allocator>::map(F&& f, Args&&... args)
    {
        using ReturnType = std::invoke_result_t<F, T&, Args...>;
        return *this |
            std::views::transform(
                [&](T& item)
                { return std::invoke(f, item, args...); }) |
            std::ranges::to<Vec<ReturnType>>();
    }

    template <typename T, template <typename> typename Operator,
              typename Allocator>
    template <typename F, typename... Args>
        requires std::is_same_v<std::invoke_result_t<F, Args...>,
                                bool>
    Vec<T> Vec<T, Operator, Allocator>::filter(F&& f, Args&&... args)
    {
        auto a = *this |
            std::ranges::filter_view(
                [&](T& t) -> bool
                { return std::invoke(f, t, std::forward(args)...); });
        return Vec<T>(a.begin(), a.end());
    }
}
