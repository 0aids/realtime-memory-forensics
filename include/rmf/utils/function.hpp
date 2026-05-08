#ifndef functions_hpp_INCLUDED
#define functions_hpp_INCLUDED
// Custom function wrapper for threaded and mapping.
#include "rmf/utils/threadpool.hpp"
#include "rmf/logging/logging.hpp"
#include <functional>
#include <iterator>
#include <ranges>
#include <tuple>
#include <type_traits>
#include <utility>
#include "rmf/utils/meta.hpp"

namespace RealtimeMemoryForensics::Utils
{
    namespace Detail
    {

        // A temporary object that holds the information
        template <typename Output, bool Flatten>
        struct Threader
        {
            std::vector<std::move_only_function<Output()>> funcVec = {};

            auto                                           with(ThreadPool&);
        };
    }
    // Consider nttp here instead?
    template <auto Func, auto FuncThreaded = Func, bool Flatten = false>
    class Function
    {
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
        constexpr Function()                = default;
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

    // fucking disgusting for cleaner code.
    template <auto Func, auto FuncThreaded, bool Flatten>
    template <typename... Args>
    auto Function<Func, FuncThreaded, Flatten>::operator()(Args&&... args) const
    {
        return Func(std::forward<Args>(args)...);
    }

    template <auto Func, auto FuncThreaded, bool Flatten>
    template <typename InputsTuple, typename... Args, size_t N>
    auto
    Function<Func, FuncThreaded, Flatten>::threaderImpl(Args&&... args) const
    {
        // Meta::TypePrinter<VecArgsTuple, InputsTuple>  Gah;
        using VecArgsTuple = typename std::tuple<Args&&...>;
        auto   vat         = std::forward_as_tuple(args...);
        auto   idxSeq      = std::make_index_sequence<N>();
        size_t length      = 0;
        bool   isValid     = true;
        [&]<size_t... Is>(std::index_sequence<Is...>) mutable
        {
            (
                [&]()
                {
                    rmf_Ok("Pack expansions: {}", Is);
                    using VecArg    = std::tuple_element_t<Is, VecArgsTuple>;
                    using InputType = std::tuple_element_t<Is, InputsTuple>;
                    if constexpr (std::is_convertible_v<VecArg, InputType>)
                    {
                    }
                    else if constexpr (std::ranges::range<VecArg> &&
                                       std::is_convertible_v<
                                           std::ranges::range_value_t<VecArg>,
                                           std::decay_t<InputType>>)
                    {
                        size_t proposedLength = std::get<Is>(vat).size();
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
                                std::is_convertible<VecArg, InputType>,
                                std::is_convertible<
                                    std::ranges::range_value_t<VecArg>,
                                    std::decay_t<InputType>>>::value,
                            "Invalid inputs to function");
                        rmf_Error("This should not be happening!");
                    }
                }(),
                ...);
        }.template operator()(idxSeq);
        using Output =
            Meta::InvokeAndUnwrap_t<decltype(FuncThreaded), InputsTuple>;
        if (!isValid)
        {
            rmf_Error("Unequal vectorized vector inputs to function!");
            return Detail::Threader<Output, Flatten>{};
        }

        // For each arg, get the underlying argument if it's a container that's not
        // an argument of FT.
        std::vector<std::move_only_function<Output()>> inputs = {};
        inputs.reserve(length);
        rmf_Info("Length of vector: {}", length);

        for (size_t i = 0; i < length; i++)
        {
            // Make tuple of valid elements (moved).
            auto tup = [&]<size_t... Is>(
                           std::index_sequence<Is...>) mutable -> decltype(auto)
            {
                return std::forward_as_tuple(
                    [&]() mutable -> decltype(auto)
                    {
                        // GAHHHHH
                        using VecArg = std::tuple_element_t<Is, VecArgsTuple>;
                        using InputType = std::tuple_element_t<Is, InputsTuple>;
                        if constexpr (std::is_convertible_v<VecArg, InputType>)
                        {
                            return std::forward<InputType>(std::get<Is>(vat));
                        }
                        else
                        {
                            return std::forward<InputType>(
                                std::get<Is>(vat)[i]);
                        }
                    }()...);
            }.template operator()(idxSeq);
            // rmf_Debug("Tuple generated: {}", tup);

            inputs.push_back(
                [tup = std::move(tup)]() mutable
                {
                    return std::apply(std::move(FuncThreaded),
                                      std::forward<InputsTuple>(tup));
                });
            // Create lambda applying those elements.
            // Push said lambda
        }
        return Detail::Threader<Output, Flatten>{
            .funcVec = std::move(inputs),
        };
    }

    template <auto Func, auto FuncThreaded, bool Flatten>
    template <typename... Args, size_t N>
        requires(N > 0)
    constexpr auto Function<Func, FuncThreaded, Flatten>::templateThreaded(
        Args&&... args) const
    {
        // Run the compile-time search tree to find the correct unwrapping path!
        using InputsTuple = typename Meta::SignatureSearcher<
            decltype(FuncThreaded), std::tuple<>, std::tuple<Args&&...>>::type;

        // If SFINAE fails to find a path, the user gets a clean error message.
        static_assert(!std::is_same_v<InputsTuple, void>,
                      "Could not find a valid combination of direct "
                      "or unwrapped arguments! ");
        // Meta::TypePrinter<InputsTuple> gah;
        return threaderImpl<InputsTuple>(std::forward<Args>(args)...);
    }

    template <auto Func, auto FuncThreaded, bool Flatten>
    template <typename... Args, size_t N>
        requires(N > 0)
    constexpr auto
    Function<Func, FuncThreaded, Flatten>::defaultThreaded(Args&&... args) const
    {
        // We cannot use threaded if operator() is templated, as this
        // would mean that we cannot determine the inputs without evaluating
        // our inputs. So we need to somehow create a valid operator() in order
        // to get the inputs. But we can't do that without evaluating operator(),
        // unless we try every combination of Args and unwrapped Args.
        using FTTraits    = Meta::FuncTraits<decltype(FuncThreaded)>;
        using InputsTuple = typename FTTraits::InputsTuple;
        return threaderImpl<InputsTuple>(std::forward<Args>(args)...);
    }
    template <auto Func, auto FuncThreaded, bool Flatten>
    template <typename... Args, size_t N>
        requires(N > 0)
    constexpr auto
    Function<Func, FuncThreaded, Flatten>::threaded(Args&&... args) const
    {
        if constexpr (Meta::ValidSignature<decltype(FuncThreaded)>)
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
        template <typename Output, bool Flatten>
        auto Threader<Output, Flatten>::with(ThreadPool& tp)
        {
            std::vector<std::future<Output>> futures;
            futures.reserve(funcVec.size());
            for (auto&& f : funcVec)
            {
                futures.push_back(tp.pushTask(std::move(f)));
            }
            rmf_Info("Number results: {}", futures.size());
            rmf_Info("Number tasks: {}", funcVec.size());
            if constexpr (std::same_as<void, Output>)
            {
                tp.awaitTasks();
                return;
            }
            else if constexpr (Flatten)
            {
                // Use the host thread to flatten constantly as results are gotten.
                using UnderlyingType = std::ranges::range_value_t<Output>;
                std::vector<UnderlyingType> result;
                for (auto& rFut : futures)
                {
                    auto value = rFut.get();
                    std::move(value.begin(), value.end(),
                              std::back_inserter(result));
                }
                return result;
            }
            else
                return futures |
                       std::views::transform([](auto& f) { return f.get(); }) |
                       std::ranges::to<std::vector<Output>>();
        }
    }
}
#endif // functions_hpp_INCLUDED
