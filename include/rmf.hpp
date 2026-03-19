#ifndef rmf_hpp_INCLUDED
#define rmf_hpp_INCLUDED
// Holds the main API calls.
#include "logger.hpp"
#include "multi_threading.hpp"
#include "types.hpp"
#include "utils.hpp"
#include <ranges>
#include <tuple>
#include <type_traits>
#include <utility>

namespace rmf
{
    template <typename InnerResultType>
    class AnalyzerResult : public std::vector<InnerResultType>
    {
      public:
        using std::vector<InnerResultType>::vector;

        // Only flattens at most 2d. If you somehow get 3d arrays then
        // unlucky.
        auto flatten()
        {
            if constexpr (std::ranges::input_range<
                              std::ranges::range_value_t<
                                  decltype(*this)>>)
            {
                using InnerContainer =
                    std::ranges::range_value_t<decltype(*this)>;
                // I don't know if this will cause large copies. I hope not because
                // this is basically magic to me.
                // I trust in RVO
                return *this | std::views::join |
                    std::ranges::to<InnerContainer>();
            }
            else
            {
                rmf_Log(rmf_Error,
                        "Unable to flatten analyzer result, not "
                        "nested containers, returning original");
                return *this;
            }
        }

        // Implicit conversion from vectors.
        AnalyzerResult(std::vector<InnerResultType>&& other) :
            std::vector<InnerResultType>(other) {};
        // We don't want implicit copy as it could be slow??
        explicit AnalyzerResult(
            const std::vector<InnerResultType>& other) :
            std::vector<InnerResultType>(other) {};
    };

    template <typename T>
    struct Vectorize
    {
        using type = T;
    };

    template <>
    struct Vectorize<types::MemoryRegionProperties>
    {
        using type = types::MemoryRegionPropertiesVec;
    };

    template <>
    struct Vectorize<types::MemorySnapshot>
    {
        using type = types::MemorySnapshotVec;
    };

    template <typename T>
    using Vectorize_t = typename Vectorize<T>::type;

    template <typename Tuple>
    struct TupleVectorize;

    template <typename... Ts>
    struct TupleVectorize<std::tuple<Ts...>>
    {
        using typeTuple = std::tuple<Vectorize_t<std::decay_t<Ts>>...>;
    };

    // Abstraction over operations and threadpooling.
    class Analyzer
    {
      private:
        // Handle-body because might be handled by python.
        struct Impl
        {
            TaskThreadPool_t tp;
            Impl(size_t numThreads) : tp(numThreads) {}
        };
        std::shared_ptr<Impl> m_impl;

        // template struct declaration to figure out types about functions
        template <typename T>
        struct functionTraits
            : functionTraits<decltype(&T::operator())>
        {
        };

        // Base specialization for free functions
        template <typename R, typename... Args>
        struct functionTraits<R(Args...)>
        {
            using returnType = R;
            using argsTuple  = std::tuple<Args...>;
        };

        // For function pointers
        template <typename R, typename... Args>
        struct functionTraits<R (*)(Args...)>
            : functionTraits<R(Args...)>
        {
        };

        // For std::function
        template <typename R, typename... Args>
        struct functionTraits<std::function<R(Args...)>>
            : functionTraits<R(Args...)>
        {
        };

        // For member functions and lambdas (const)
        template <typename C, typename R, typename... Args>
        struct functionTraits<R (C::*)(Args...) const>
            : functionTraits<R(Args...)>
        {
        };

        // For member functions and lambdas (non-const mutable)
        template <typename C, typename R, typename... Args>
        struct functionTraits<R (C::*)(Args...)>
            : functionTraits<R(Args...)>
        {
        };

        // See Analyzer::Execute to get a brief overview of how it works
        template <typename func_t, typename RecvArgsTuple_t,
                  size_t... Is>
        auto ExecuteImpl(func_t&& func, RecvArgsTuple_t&& argsRecv,
                         std::index_sequence<Is...>)
        {
            using traits = functionTraits<std::decay_t<func_t>>;
            using ActualArgsTuple_t = traits::argsTuple;
            using Return_t          = typename traits::returnType;
            bool                     error = false;
            AnalyzerResult<Return_t> result;
            size_t                   vecSize = 1;
            (
                [&]() mutable
                {
                    using actualArg = std::remove_cvref_t<
                        std::tuple_element_t<Is, ActualArgsTuple_t>>;
                    using recvArg = std::remove_cvref_t<
                        std::tuple_element_t<Is, RecvArgsTuple_t>>;
                    if constexpr (std::is_convertible_v<recvArg,
                                                        actualArg>)
                    {
                    }
                    else if constexpr (
                        utils::IsContainer<recvArg> &&
                        std::is_convertible_v<
                            typename recvArg::value_type, actualArg>)
                    {
                        if (vecSize !=
                                std::get<Is>(argsRecv).size() &&
                            vecSize != 1)
                        {
                            rmf_Log(
                                rmf_Error,
                                "Differing container input sizes "
                                "to Analyzer::Execute!"
                                " Got: "
                                    << vecSize << ", "
                                    << std::get<Is>(argsRecv).size());
                            error = true;
                            return;
                        }
                        vecSize = std::get<Is>(argsRecv).size();
                    }
                    else
                    {
                        utils::typePrinter<RecvArgsTuple_t, actualArg,
                                           recvArg>();
                        static_assert(false,
                                      "invalid inputs to function in "
                                      "Analyzer::Execute!");
                    }
                }(),
                ...);

            if (error)
                return result;
            std::vector<Task_t<Return_t>> taskList;

            for (size_t i = 0; i < vecSize; i++)
            {
                auto tuple = std::make_tuple(
                    func,
                    [&]() mutable
                    {
                        using actualArg =
                            std::remove_cvref_t<std::tuple_element_t<
                                Is, ActualArgsTuple_t>>;
                        using recvArg =
                            std::remove_cvref_t<std::tuple_element_t<
                                Is, RecvArgsTuple_t>>;
                        if constexpr (std::is_convertible_v<
                                          recvArg, actualArg>)
                            return std::get<Is>(argsRecv);
                        else if constexpr (
                            utils::IsContainer<recvArg> &&
                            std::is_convertible_v<
                                typename recvArg::value_type,
                                actualArg>)
                            return std::get<Is>(argsRecv)[i];
                        else
                        {
                            static_assert(false, "what?");
                        }
                    }()...);
                taskList.push_back(
                    std::make_from_tuple<Task_t<Return_t>>(tuple));
                m_impl->tp.SubmitTask(taskList.back());
            }

            for (auto& task : taskList)
            {
                result.push_back(task.getFuture().get());
            }
            return result;
        }

      public:
        Analyzer(size_t numThreads);
        std::shared_ptr<Impl> getImpl()
        {
            return m_impl;
        }

        // Given a function, that takes some arguments A...,
        // apply those argument s.t. if some "a" is a container,
        // it creates tasks func(a1, b, c), func(a2, b, c) and so on.
        /* Basic overview of how it works:
             * Using compile time intrinsics, we convert variadic templates
             * into a tuple of types.
             * For each of the types, we compare it against the function argument, and
             * check whether the types are the same, or the type is a container of the
             * original type.
             *
             * If it is a container of the original type, we will store its length,
             * and use it to create tasks such that we have a task using one element
             * of the vector, and then the rest.
             *
             * To do so we use std::make_index_sequence to allow us to std::get
             * all of the tuples, ensuring we can then double check types against
             * each other.
             *
             * The hardest part about this was ironing all the tiny ass errors with forgetting _t,
             * removing Const and reference, etc.
         * */
        template <typename func_t, typename... Args>
        auto Execute(func_t&& func, Args&&... args)
        {

            // for each type, check if it is a container, and also check that
            // it's a argument of the std::tuple.
            return ExecuteImpl(
                std::forward<func_t>(func),
                std::forward_as_tuple(std::forward<Args>(args)...),
                std::make_index_sequence<sizeof...(Args)>{});
        }

        template <typename func_t, typename... Args>
        static auto CreateExecution(func_t func)
        {
            return [func](rmf::Analyzer& self, Args... args)
            { return self.Execute(func, std::forward<Args>(args)...); };
        }

        // Only requires the function as the argument
        // Of which it will convert the types
        template <typename func_t>
        static auto CreateVectorisedExecution(func_t func)
        {
            // get args of function
            using traits = functionTraits<std::decay_t<func_t>>;
            using actualArgsTuple = traits::argsTuple;
            using newArgsTuple =
                TupleVectorize<actualArgsTuple>::typeTuple;

            auto indexHelper =
                [func]<size_t... Is>(std::index_sequence<Is...>)
            {
                return Analyzer::CreateExecution<
                    func_t,
                    std::remove_cvref_t<std::tuple_element_t<Is, newArgsTuple>>...>(func);
            };
            return indexHelper(std::make_index_sequence<
                               std::tuple_size_v<newArgsTuple>>{});
        }
    };
}

#endif // rmf_hpp_INCLUDED
