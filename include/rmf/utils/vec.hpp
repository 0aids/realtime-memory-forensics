#pragma once
#include "rmf/node.hpp"
#include "rmf/utils/expect.hpp"
#include "rmf/utils/function.hpp"
#include "rmf/utils/threadpool.hpp"
#include <algorithm>
#include <cassert>
#include <concepts>
#include <functional>
#include <iterator>
#include <optional>
#include <ranges>
#include <memory>
#include <type_traits>
#include <vector>
namespace rmf::Utils
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

    struct Pipe
    {
        struct End
        {
        };
        struct EndThreaded
        {
            ThreadPool& tp;
        };
        // We cannot convert it to the same T
        template <typename T, typename Pipeline = std::false_type>
        struct Impl
        {
            T&       data;
            Pipeline pipe;
            auto     operator|(const auto F)
                requires(!std::same_as<std::decay_t<decltype(F)>, End> &&
                         !std::same_as<std::decay_t<decltype(F)>, EndThreaded>);
            auto operator|(End);
            auto operator|(EndThreaded);
        };
    };

    template <typename T, typename Operator = VecOpTraits<T>::type,
              typename Allocator = std::allocator<T>>
    class Vec : public std::vector<T, Allocator>,
                public Operator,
                public Utils::Error
    {
      public:
        using BaseType = std::vector<T, Allocator>;
        using BaseType::BaseType;
        using InnerType = T;
        using SelfType  = Vec<T, Operator, Allocator>;

        Vec(BaseType&& base);

        template <typename F, typename... Args>
        auto map(F&& f, Args&&... args);

        template <auto F, typename... Args>
        auto mapThreaded(Args&&... args);

        template <typename F, typename... Args>
            requires std::is_same_v<std::invoke_result_t<F, Args...>, bool>
        Vec<T> filter(F&& f, Args&&... args);

        template <typename InnerInnerType>
        Vec<InnerInnerType> flatten();

        auto                pipe();
    };
}

namespace rmf::Utils
{
    template <typename T, typename Operator, typename Allocator>
    template <typename F, typename... Args>
    auto Vec<T, Operator, Allocator>::map(F&& f, Args&&... args)
    {
        using ReturnType = std::invoke_result_t<F, T&, Args...>;
        if constexpr (std::same_as<ReturnType, void>)
        {
            // hmmm.
            for (auto& m : *this)
            {
                // hmm forward doesn't work here?
                f(m, std::forward<Args>(args)...);
            }
        }
        else
            return *this |
                   std::views::transform(
                       [&](T& item) { return std::invoke(f, item, args...); }) |
                   std::ranges::to<Vec<ReturnType>>();
    }

    template <typename T, typename Operator, typename Allocator>
    template <typename F, typename... Args>
        requires std::is_same_v<std::invoke_result_t<F, Args...>, bool>
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

    // Threaded wrapper!
    template <typename T, typename Operator, typename Allocator>
    template <auto F, typename... Args>
    auto Vec<T, Operator, Allocator>::mapThreaded(Args&&... args)
    {
        return Utils::Function<F, F, false>().threaded(
            *this, std::forward<Args>(args)...);
    }
    template <typename T, typename Operator, typename Allocator>
    auto Vec<T, Operator, Allocator>::pipe()
    {
        return Pipe::Impl{
            .data = *this,
            .pipe = {},
        };
    }
    template <typename T, typename Operator, typename Allocator>
    Vec<T, Operator, Allocator>::Vec(BaseType&& base) :
        std::vector<T, Allocator>(std::move(base))
    {
    }
    template <typename T, typename Pipeline>
    auto Pipe::Impl<T, Pipeline>::operator|(const auto F)
        requires(!std::same_as<std::decay_t<decltype(F)>, End> &&
                 !std::same_as<std::decay_t<decltype(F)>, EndThreaded>)
    {
        if constexpr (!std::same_as<Pipeline, std::false_type>)
        {
            auto newPipeline = pipe | std::views::transform(F);
            return Impl<T, decltype(newPipeline)>{.data = data,
                                                  .pipe = newPipeline};
        }
        else
        {
            auto p = std::views::transform(F);
            return Impl<T, decltype(p)>{.data = data, .pipe = p};
        }
    }

    template <typename T, typename Pipeline>
    auto Pipe::Impl<T, Pipeline>::operator|(End)
    {
        if constexpr (!std::same_as<Pipeline, std::false_type>)
        {
            // Check what the last pipeline's result is
            using pipelineResult =
                std::invoke_result_t<decltype(pipe), decltype(data)>;
            using pipelineUnderlying =
                std::ranges::range_value_t<pipelineResult>;
            return data | pipe |
                   std::ranges::to<Utils::Vec<pipelineUnderlying>>();
        }
        else
        {
            return data;
        }
    }

    template <typename T, typename Pipeline>
    auto Pipe::Impl<T, Pipeline>::operator|(EndThreaded)
    {
        assert(false && "TODO!");
        using pipelineResult =
            std::invoke_result_t<decltype(pipe), decltype(data)>;
        using pipelineUnderlying = std::ranges::range_value_t<pipelineResult>;
        return Utils::Vec<pipelineUnderlying>{};
    }
}
