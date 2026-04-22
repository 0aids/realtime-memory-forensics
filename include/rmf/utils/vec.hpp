#pragma once
#include <algorithm>
#include <functional>
#include <iterator>
#include <ranges>
#include <memory>
#include <type_traits>
#include <vector>
namespace RealtimeMemoryForensics::Utils
{
    template <typename T, typename Allocator = std::allocator<T>>
    class Vec : public std::vector<T, Allocator>
    {
      private:
      public:
        using BaseType = std::vector<T, Allocator>;
        using BaseType::BaseType;

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
    template <typename T, typename Allocator>
    template <typename F, typename... Args>
    auto Vec<T, Allocator>::map(F&& f, Args&&... args)
    {
        using ReturnType = std::invoke_result_t<F, T&, Args...>;
        return *this |
            std::views::transform(
                [&](T& item)
                { return std::invoke(f, item, args...); }) |
            std::ranges::to<Vec<ReturnType>>();
    }

    template <typename T, typename Allocator>
    template <typename F, typename... Args>
        requires std::is_same_v<std::invoke_result_t<F, Args...>,
                                bool>
    Vec<T> Vec<T, Allocator>::filter(F&& f, Args&&... args)
    {
        auto a = *this |
            std::ranges::filter_view(
                [&](T& t) -> bool
                { return std::invoke(f, t, std::forward(args)...); });
        return Vec<T>(a.begin(), a.end());
    }
}
