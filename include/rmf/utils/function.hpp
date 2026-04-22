#ifndef functions_hpp_INCLUDED
#define functions_hpp_INCLUDED
// Custom function wrapper for threaded and mapping.
#include "rmf/utils/vec.hpp"
#include "rmf/utils/threadpool.hpp"
#include "rmf/logging/logging.hpp"
#include <functional>
#include <ranges>
#include <tuple>
#include <type_traits>
#include <utility>

namespace RealtimeMemoryForensics::Utils
{
    namespace Detail
    {
        template <typename... T>
        struct TypePrinter;

        template <typename T>
        struct FunctionTraitsGetter
        {
            static_assert(
                false,
                "Type T is not a function, or not decoded via "
                "a FunctionDecoder!");
        };

        template <typename R, typename... Args>
        struct FunctionTraitsGetter<std::function<R(Args...)>>
        {
            using Base               = std::function<R(Args...)>;
            using InputsTuple        = std::tuple<Args...>;
            using ForwardInputsTuple = std::tuple<Args&&...>;
            using Output             = R;
            using Signature          = R(Args...);
            using FuncPtr            = R (*)(Args...);
        };

        template <typename F>
        using FunctionDecoder =
            decltype(std::function{std::declval<F>()});

        template <typename F>
        using FuncTraits = FunctionTraitsGetter<FunctionDecoder<F>>;

        // A temporary object that holds the information
        template <typename FT>
        struct Threader
        {
            using Output = typename FuncTraits<FT>::Output;
            Vec<std::move_only_function<Output()>> funcVec = {};

            Vec<Output>                            with(ThreadPool&);
        };
    }

    template <typename F, typename FT = F>
    class Function
    {
        F  m_func;
        FT m_mtFunc;

      public:
        using FTraits  = Detail::FuncTraits<F>;
        using FTTraits = Detail::FuncTraits<FT>;

      public:
        Function(F implFunction);
        Function(F implFunction, FT threadedFunction);
        Function(Function&&)                = delete;
        Function(const Function&)           = delete;
        Function operator=(const Function&) = delete;
        Function operator=(Function&&)      = delete;

        // For actually running the function
        // GAHHHHH
        template <typename... Args>
            requires std::is_convertible_v<
                std::tuple<Args...>, typename FTraits::InputsTuple>
        typename FTraits::Output operator()(Args&&...);

        // Returns an intermediate object that contains a "with" method.
        template <typename... Args,
                  size_t N = std::tuple_size_v<std::tuple<Args...>>>
        auto threaded(Args&&... args);

        // Runs very basic parallelised (but not threaded)
        template <typename... Args>
        Vec<typename FTraits::Output> applyTo(Args&&... args);
    };
}

namespace RealtimeMemoryForensics::Utils
{
    template <typename F, typename FT>
    Function<F, FT>::Function(F implFunction) :
        m_func(implFunction), m_mtFunc(implFunction)
    {
    }

    // fucking disgusting for cleaner code.
    template <typename F, typename FT>
    template <typename... Args>
        requires std::is_convertible_v<
            std::tuple<Args...>,
            typename Detail::FuncTraits<F>::InputsTuple>
    typename Detail::FuncTraits<F>::Output
    Function<F, FT>::operator()(Args&&... args)
    { return m_func(std::forward<Args>(args)...); }

    template <typename F, typename FT>
    template <typename... Args, size_t N>
    auto Function<F, FT>::threaded(Args&&... args)
    {
        using InputsTuple  = typename FTTraits::InputsTuple;
        using VecArgsTuple = typename std::tuple<Args...>;
        auto   vat         = std::forward_as_tuple<Args...>(args...);
        auto   idxSeq      = std::make_index_sequence<N>();
        size_t length      = 0;
        bool   isValid     = true;
        // List of bools to indicate which types are vectorised.
        [&]<size_t... Is>(std::index_sequence<Is...>) mutable
        {
            (
                [&]()
                {
                    rmf_Ok("Pack expansions: {}", Is);
                    using VecArg =
                        std::tuple_element_t<Is, VecArgsTuple>;
                    using InputType =
                        std::tuple_element_t<Is, InputsTuple>;
                    if constexpr (std::is_convertible_v<VecArg,
                                                        InputType>)
                    {
                    }
                    else if constexpr (std::ranges::range<VecArg> &&
                                       std::is_convertible_v<
                                           std::ranges::range_value_t<
                                               VecArg>,
                                           InputType>)
                    {
                        size_t proposedLength =
                            std::get<Is>(vat).size();
                        if (length != 0 && length != proposedLength)
                            isValid = false;
                        else
                            length = proposedLength;
                    }
                    else
                    {
                        static_assert(false,
                                      "Invalid inputs to function");
                    }
                }(),
                ...);
        }.template operator()(idxSeq);
        if (!isValid)
        {
            rmf_Error(
                "Unequal vectorized vector inputs to function!");
            return Detail::Threader<FT>{};
        }
        // For each arg, get the underlying argument if it's a container that's not
        // an argument of FT.
        using Output = typename FTTraits::Output;
        Vec<std::move_only_function<Output()>> inputs = {};
        inputs.reserve(length);
        rmf_Info("Length of vector: {}", length);

        for (size_t i = 0; i < length; i++)
        {
            // Make tuple of valid elements (moved).
            auto tup =
                [&]<size_t... Is>(std::index_sequence<Is...>) mutable
                -> decltype(auto)
            {
                return std::forward_as_tuple(
                    [&]() mutable -> decltype(auto)
                    {
                        // GAHHHHH
                        using VecArg =
                            std::tuple_element_t<Is, VecArgsTuple>;
                        using InputType =
                            std::tuple_element_t<Is, InputsTuple>;
                        if constexpr (std::is_convertible_v<
                                          VecArg, InputType>)
                        {
                            if constexpr (std::is_rvalue_reference_v<
                                              VecArg>)
                                return std::move(std::get<Is>(vat));
                            return std::move(std::get<Is>(vat));
                        }
                        else
                        {
                            return std::move(std::get<Is>(vat)[i]);
                        }
                    }()...);
            }.template operator()(idxSeq);
            // rmf_Debug("Tuple generated: {}", tup);

            inputs.push_back(
                [mt_func = std::move(m_mtFunc),
                 tup     = std::move(tup)]() mutable
                {
                    return std::apply(std::move(mt_func),
                                      std::move(tup));
                });
            // Create lambda applying those elements.
            // Push said lambda
        }
        return Detail::Threader<FT>{
            .funcVec = std::move(inputs),
        };
    }
    namespace Detail
    {
        template <typename FT>
        Vec<typename FuncTraits<FT>::Output>
        Threader<FT>::with(ThreadPool& tp)
        {
            Vec<std::future<Output>> results;
            results.reserve(funcVec.size());
            for (auto&& f : funcVec)
            {
                results.push_back(tp.pushTask(std::move(f)));
            }
            tp.awaitTasks();
            rmf_Info("Number results: {}", results.size());
            rmf_Info("Number tasks: {}", funcVec.size());
            return results.map(&std::future<Output>::get);
        }
    }
}
#endif // functions_hpp_INCLUDED
