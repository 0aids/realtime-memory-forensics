#pragma once
#include "rmf/node.hpp"
#include "rmf/utils/function.hpp"
#include <algorithm>
#include <concepts>
#include <functional>
#include <iterator>
#include <ranges>
#include <memory>
#include <type_traits>
#include <vector>
namespace RealtimeMemoryForensics::Utils
{
    // Mixins for providing vectorised operations.
    class DefaultOperator
    {
    };

    template <typename T, typename = void>
    struct VecOpTraits
    {
        using type = DefaultOperator;
    };

    template <typename... Features>
    struct VecOpTraits<Node<Features...>>
    {
        using type = Node<Features...>::VecOp;
    };

    template <typename T, typename Operator = VecOpTraits<T>::type,
              typename Allocator = std::allocator<T>>
    class Vec : public std::vector<T, Allocator>, public Operator
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

        template <typename InnerInnerType>
        Vec<InnerInnerType> flatten();
    };
}

namespace RealtimeMemoryForensics::Utils
{
    template <typename T, typename Operator, typename Allocator>
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

    template <typename T, typename Operator, typename Allocator>
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

    template <typename T, typename Operator, typename Allocator>
    template <typename InnerInnerType>
    Vec<InnerInnerType> Vec<T, Operator, Allocator>::flatten()
    {
        Vec<InnerInnerType> result;
        for (auto& inner : *this)
        {
            for (auto& innerInner : inner)
            {
                result.emplace_back(std::move(innerInner));
            }
        }
        return result;
    }
}
