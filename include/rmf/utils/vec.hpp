#pragma once
#include "rmf/node.hpp"
#include "rmf/utils/expect.hpp"
#include "rmf/utils/function.hpp"
#include "rmf/utils/threadpool.hpp"
#include "rmf/utils/meta.hpp"
#include <algorithm>
#include <cassert>
#include <concepts>
#include <functional>
#include <iterator>
#include <optional>
#include <ranges>
#include <memory>
#include <span>
#include <tuple>
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
            T&       m_data;
            Pipeline m_pipe;
            auto     operator|(auto&& F)
                requires(!std::same_as<std::decay_t<decltype(F)>, End> &&
                         !std::same_as<std::decay_t<decltype(F)>, EndThreaded>);

            auto operator|(End&&);

            auto operator|(EndThreaded&& et);
        };
    };

    template <typename T, typename Operator = VecOpTraits<T>::type,
              typename Allocator = std::allocator<T>>
    class Vec : public std::vector<T, Allocator>, public Operator
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
            .m_data = *this,
            .m_pipe = {},
        };
    }
    template <typename T, typename Operator, typename Allocator>
    Vec<T, Operator, Allocator>::Vec(BaseType&& base) :
        std::vector<T, Allocator>(std::move(base))
    {
    }
    template <typename T, typename Pipeline>
    auto Pipe::Impl<T, Pipeline>::operator|(auto&& F)
        requires(!std::same_as<std::decay_t<decltype(F)>, End> &&
                 !std::same_as<std::decay_t<decltype(F)>, EndThreaded>)
    {
        // If we're the first one, and we're a zip, the perform the correct shenanigans.
        // To allow unzipping.
        if constexpr (!std::same_as<Pipeline, std::false_type>)
        {
            // Consider adding a mini-consolidation depending on the operation?
            auto newPipeline = m_pipe | std::views::transform(F);
            return Impl<T, decltype(newPipeline)>{.m_data = m_data,
                                                  .m_pipe = newPipeline};
        }
        else if constexpr (Meta::isTemplatedFrom<T, std::ranges::zip_view>)
        {
            auto newF = [F](auto&& val) mutable { return std::apply(F, val); };

            static_assert(
                Meta::isTemplatedFrom<decltype(m_data.front()), std::tuple>,
                "Inside zips should be tuples");
            static_assert(
                requires { newF(m_data.front()); },
                "Zipped data should be able to be inputted directly");

            auto newPipeline = std::views::transform(newF);
            return Impl<T, decltype(newPipeline)>{.m_data = m_data,
                                                  .m_pipe = newPipeline};
        }
        else
        {
            auto p = std::views::transform(F);
            return Impl<T, decltype(p)>{.m_data = m_data, .m_pipe = p};
        }
    }

    template <typename T, typename Pipeline>
    auto Pipe::Impl<T, Pipeline>::operator|(End&&)
    {
        // Zipping. We can zip some amount of values and then apply them.
        // If we detect a zip, then we should probably just apply them?
        if constexpr (!std::same_as<Pipeline, std::false_type>)
        {
            auto res = m_data | m_pipe;
            return Vec(std::vector(res.begin(), res.end()));
        }
        else
        {
            return m_data;
        }
    }

    template <typename T, typename Pipeline>
    auto Pipe::Impl<T, Pipeline>::operator|(EndThreaded&& et)
    {
        // Alternative solution:
        // Piping solution 1: Store all functors as a vector of std::function and then use that.
        // Pros - Compatible with python
        // Cons - Slower, more runtime-dependent.
        // 		- Have to type-erase everything, and piping different inputs becomes difficult.
        // Piping solution 2: Switch to an eager activation model
        // Pros - Compatible with python
        // Cons - Api change
        // 		- Slower, less optimization available (i assume?)
        // Piping solution 3: Implement a slightly custom piping solution in python.
        // Piping solution 4: No piping solution, just split up the vector into chunks for
        // each thread, and add that as a task, then perform consolidation.
        // Pros - Easiest implementation
        // Cons - Not the best for python.
        // I think for python I will most likely end up using the fattest nodes possible by default.
        // As for pipe operations, I think that i'll have a custom solution written in python.
        // Piping solution 4 will be the one chosen.
        // assert(false && "TODO!");
        using pipelineResult =
            std::invoke_result_t<decltype(m_pipe), decltype(m_data)>;
        using pipelineUnderlying = std::ranges::range_value_t<pipelineResult>;
        const size_t numThreads  = et.tp.getNumThreads();
        // Split up the data somewhat evenly and then apply the pipeline.
        const size_t workPerThread = m_data.size() / numThreads;
        auto         resultLazy    = m_data | std::views::chunk(workPerThread);
        std::vector<std::future<Utils::Vec<pipelineUnderlying>>> futuresVector;
        Utils::Vec<pipelineUnderlying>                           finalResult;
        for (auto chunk : resultLazy)
        {
            futuresVector.push_back(et.tp.pushTask(
                [chunk = std::move(chunk), this]() mutable
                {
                    return chunk | m_pipe |
                           std::ranges::to<Utils::Vec<pipelineUnderlying>>();
                }));
        }
        // Consolidate all the data.
        auto backInserter = std::back_inserter(finalResult);
        for (auto& fut : futuresVector)
        {
            auto futRes = fut.get();
            std::move(futRes.begin(), futRes.end(), backInserter);
        }
        return finalResult;
    }
}
