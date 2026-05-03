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
    template <typename F, typename FT = F>
    class Function
    {
        F  m_func;
        FT m_mtFunc;
        template <typename InputsTuple, typename... Args,
                  size_t N = std::tuple_size_v<std::tuple<Args...>>>
        auto threaderImpl(Args&&... args) const;

        template <typename... Args,
                  size_t N = std::tuple_size_v<std::tuple<Args...>>>
            requires(N > 0)
        constexpr auto defaultThreaded(Args&&... args) const;

        template <typename... Args,
                  size_t N = std::tuple_size_v<std::tuple<Args...>>>
            requires(N > 0)
        constexpr auto templateThreaded(Args&&... args) const;

      public:
        constexpr Function(F implFunction);
        constexpr Function(F implFunction, FT threadedFunction);
        Function(Function&&)                = delete;
        Function(const Function&)           = delete;
        Function operator=(const Function&) = delete;
        Function operator=(Function&&)      = delete;

        // For actually running the function
        // Runs the function inputted like normal.
        template <typename... Args>
        auto operator()(Args&&...) const;

        // Returns an intermediate object that contains a "with" method.
        // Does this by automatically parallelising arguments that are containers of the function's
        // input types. IE if a function takes in a vector and a scalar, and you pass it a vector of vector and singular
        // scalar, it will parallelise only over the vector while doing copies for the singular.
        template <typename... Args,
                  size_t N = std::tuple_size_v<std::tuple<Args...>>>
            requires(N > 0)
        constexpr auto threaded(Args&&... args) const;

        // Runs very basic parallelised (but not threaded)
        template <typename... Args>
        auto applyTo(Args&&... args) const;
    };
}

namespace RealtimeMemoryForensics::Utils
{
    namespace Detail
    {
        template <typename... T>
        struct TypePrinter;

        template <typename T>
        struct FunctionTraitsGetter
        {
            using Valid = std::false_type;
            // static_assert(
            //     false,
            //     "Type T is not a function, or not decoded via "
            //     "a FunctionDecoder!");
        };

        template <typename R, typename... Args>
        struct FunctionTraitsGetter<std::function<R(Args...)>>
        {
            using Valid              = std::true_type;
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

        template <typename T>
        concept ValidSignature =
            requires { std::function{std::declval<T>()}; };

        // A temporary object that holds the information
        template <typename Output>
        struct Threader
        {
            Vec<std::move_only_function<Output()>> funcVec = {};

            Vec<Output>                            with(ThreadPool&);
        };

        // Helper to unwrap one level of a range, preserving references.
        // If we unwrap Vec<int>&, we get int&.
        template <typename T>
        struct UnwrapArg
        {
            using type = T;
        };

        template <std::ranges::range T>
        struct UnwrapArg<T>
        {
            using type =
                std::ranges::range_reference_t<std::decay_t<T>>;
        };

        template <typename Functor, typename AccTuple,
                  typename RemTuple>
        struct SignatureSearcher;

        // Base case: No more arguments left. Test if the combination is valid.
        template <typename Functor, typename... Acc>
        struct SignatureSearcher<Functor, std::tuple<Acc...>,
                                 std::tuple<>>
        {
            static constexpr bool is_valid =
                std::is_invocable_v<Functor, Acc...>;
            using type = std::conditional_t<is_valid,
                                            std::tuple<Acc...>, void>;
        };

        // Recursive case: Branch on the next argument
        template <typename Functor, typename... Acc, typename NextArg,
                  typename... Rest>
        struct SignatureSearcher<Functor, std::tuple<Acc...>,
                                 std::tuple<NextArg, Rest...>>
        {

            // Branch 1: Try treating the argument as a scalar/direct pass
            using TryDirect = typename SignatureSearcher<
                Functor, std::tuple<Acc..., NextArg>,
                std::tuple<Rest...>>::type;

            // Branch 2: Try treating the argument as a vector to be unwrapped
            using UnwrappedType =
                typename UnwrapArg<std::decay_t<NextArg>>::type;

            using TryUnwrapped = std::conditional_t<
                std::is_same_v<std::decay_t<NextArg>,
                               std::decay_t<UnwrappedType>>,
                void, // Skip Branch 2 if it's not actually a range
                typename SignatureSearcher<
                    Functor, std::tuple<Acc..., UnwrappedType>,
                    std::tuple<Rest...>>::type>;

            // Return the first valid signature (Direct takes priority over Unwrapped)
            using type =
                std::conditional_t<!std::is_same_v<TryDirect, void>,
                                   TryDirect, TryUnwrapped>;
        };
        template <typename Func, typename Tuple>
        struct InvokeAndUnwrap;

        template <typename Func, typename... Inputs>
        struct InvokeAndUnwrap<Func, std::tuple<Inputs...>>
        {
            using Type = std::invoke_result_t<Func, Inputs...>;
        };
        template <typename Func, typename Tuple>
        using InvokeAndUnwrap_t =
            typename InvokeAndUnwrap<Func, Tuple>::Type;
    }
    template <typename F, typename FT>
    constexpr Function<F, FT>::Function(F implFunction) :
        m_func(implFunction), m_mtFunc(implFunction)
    {
    }

    // fucking disgusting for cleaner code.
    template <typename F, typename FT>
    template <typename... Args>
    auto Function<F, FT>::operator()(Args&&... args) const
    {
        return m_func(std::forward<Args>(args)...);
    }

    template <typename F, typename FT>
    template <typename InputsTuple, typename... Args, size_t N>
    auto Function<F, FT>::threaderImpl(Args&&... args) const
    {
        // Detail::TypePrinter<VecArgsTuple, InputsTuple>  Gah;
        using VecArgsTuple = typename std::tuple<Args&&...>;
        auto   vat         = std::forward_as_tuple(args...);
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
                                           std::decay_t<InputType>>)
                    {
                        size_t proposedLength =
                            std::get<Is>(vat).size();
                        rmf_Info("Proposed length: {}, current: {}",
                                 proposedLength, length);
                        if (length != 0 && length != proposedLength)
                            isValid = false;
                        else
                            length = proposedLength;
                    }
                    else
                    {
                        static_assert(
                            std::disjunction<
                                std::is_convertible<VecArg,
                                                    InputType>,
                                std::is_convertible<
                                    std::ranges::range_value_t<
                                        VecArg>,
                                    std::decay_t<InputType>>>::value,
                            "Invalid inputs to function");
                        rmf_Error("This should not be happening!");
                    }
                }(),
                ...);
        }.template operator()(idxSeq);
        using Output = Detail::InvokeAndUnwrap_t<FT, InputsTuple>;
        if (!isValid)
        {
            rmf_Error(
                "Unequal vectorized vector inputs to function!");
            return Detail::Threader<Output>{};
        }

        // For each arg, get the underlying argument if it's a container that's not
        // an argument of FT.
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
        return Detail::Threader<Output>{
            .funcVec = std::move(inputs),
        };
    }

    template <typename F, typename FT>
    template <typename... Args, size_t N>
        requires(N > 0)
    constexpr auto
    Function<F, FT>::templateThreaded(Args&&... args) const
    {
        // Run the compile-time search tree to find the correct unwrapping path!
        using InputsTuple = typename Detail::SignatureSearcher<
            FT, std::tuple<>, std::tuple<Args&&...>>::type;

        // If SFINAE fails to find a path, the user gets a clean error message.
        static_assert(!std::is_same_v<InputsTuple, void>,
                      "Could not find a valid combination of direct "
                      "or unwrapped arguments! "
                      "(Did you constrain your operator() with a "
                      "trailing return type?)");
        // Detail::TypePrinter<InputsTuple> gah;
        return threaderImpl<InputsTuple>(std::forward<Args>(args)...);
    }

    template <typename F, typename FT>
    template <typename... Args, size_t N>
        requires(N > 0)
    constexpr auto
    Function<F, FT>::defaultThreaded(Args&&... args) const
    {
        // We cannot use threaded if operator() is templated, as this
        // would mean that we cannot determine the inputs without evaluating
        // our inputs. So we need to somehow create a valid operator() in order
        // to get the inputs. But we can't do that without evaluating operator(),
        // unless we try every combination of Args and unwrapped Args.
        using FTTraits    = Detail::FuncTraits<FT>;
        using InputsTuple = typename FTTraits::InputsTuple;
        return threaderImpl<InputsTuple>(std::forward<Args>(args)...);
    }
    template <typename F, typename FT>
    template <typename... Args, size_t N>
        requires(N > 0)
    constexpr auto Function<F, FT>::threaded(Args&&... args) const
    {
        if constexpr (Detail::ValidSignature<FT>)
        {
            return defaultThreaded(std::forward<Args>(args)...);
        }
        else
        {
            return templateThreaded(std::forward<Args>(args)...);
        }
    }

    namespace Detail
    {
        template <typename Output>
        Vec<Output> Threader<Output>::with(ThreadPool& tp)
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
