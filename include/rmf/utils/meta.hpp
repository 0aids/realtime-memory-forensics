#ifndef meta_hpp_INCLUDED
#define meta_hpp_INCLUDED

#include <concepts>
#include <type_traits>
#include <functional>

namespace rmf::Meta
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
    using FunctionDecoder = decltype(std::function{std::declval<F>()});

    template <typename F>
    using FuncTraits = FunctionTraitsGetter<FunctionDecoder<F>>;

    template <typename T>
    concept ValidSignature = requires { std::function{std::declval<T>()}; };

    // Helper to unwrap one level of a range, preserving references.
    // If we unwrap std::vector<int>&, we get int&.
    template <typename T>
    struct UnwrapArg
    {
        using type = T;
    };

    template <std::ranges::range T>
    struct UnwrapArg<T>
    {
        using type = std::ranges::range_reference_t<std::decay_t<T>>;
    };

    template <typename Functor, typename AccTuple, typename RemTuple>
    struct SignatureSearcher;

    // Base case: No more arguments left. Test if the combination is valid.
    template <typename Functor, typename... Acc>
    struct SignatureSearcher<Functor, std::tuple<Acc...>, std::tuple<>>
    {
        static constexpr bool is_valid = std::is_invocable_v<Functor, Acc...>;
        using type = std::conditional_t<is_valid, std::tuple<Acc...>, void>;
    };

    // Recursive case: Branch on the next argument
    template <typename Functor, typename... Acc, typename NextArg,
              typename... Rest>
    struct SignatureSearcher<Functor, std::tuple<Acc...>,
                             std::tuple<NextArg, Rest...>>
    {

        // Branch 1: Try treating the argument as a scalar/direct pass
        using TryDirect =
            typename SignatureSearcher<Functor, std::tuple<Acc..., NextArg>,
                                       std::tuple<Rest...>>::type;

        // Branch 2: Try treating the argument as a vector to be unwrapped
        using UnwrappedType = typename UnwrapArg<std::decay_t<NextArg>>::type;

        using TryUnwrapped = std::conditional_t<
            std::is_same_v<std::decay_t<NextArg>, std::decay_t<UnwrappedType>>,
            void, // Skip Branch 2 if it's not actually a range
            typename SignatureSearcher<Functor,
                                       std::tuple<Acc..., UnwrappedType>,
                                       std::tuple<Rest...>>::type>;

        // Return the first valid signature (Direct takes priority over Unwrapped)
        using type = std::conditional_t<!std::is_same_v<TryDirect, void>,
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
    using InvokeAndUnwrap_t = typename InvokeAndUnwrap<Func, Tuple>::Type;

    template <typename T>
    concept Numeric = requires {
        requires std::is_integral_v<T> || std::is_floating_point_v<T>;
        requires !std::is_same_v<bool, T>;
        requires !std::is_pointer_v<T>;
    };

    template <typename TargetType, typename... Args>
    concept HasType = requires {
        std::disjunction<std::is_same<TargetType, Args>...>::value;
    };

    template <typename T, typename... Rest>
    struct IsInPack : std::disjunction<std::is_same<T, Rest>...>
    {
    };

    template <typename... Args>
    struct HasDuplicates : std::false_type
    {
    };

    template <typename First, typename... Rest>
    struct HasDuplicates<First, Rest...>
        : std::disjunction<IsInPack<First, Rest...>, HasDuplicates<Rest...>>
    {
    };

    template <typename... Args>
    inline constexpr bool HasDuplicates_v = HasDuplicates<Args...>::value;

    static_assert(HasDuplicates_v<int, float, char> == false);
    static_assert(HasDuplicates_v<int, float, int> == true);

    template <typename T, typename... Exclusive>
    static constexpr int countExclusives()
    {
        return (std::same_as<T, Exclusive> + ...);
    }

    template <typename... Exclusives>
    struct RequireExclusive
    {
        template <typename... Args>
        static constexpr bool isExclusive()
        {
            return (countExclusives<Args, Exclusives...>() + ...) <= 1;
        }
    };

    static_assert(!RequireExclusive<int, float>::isExclusive<int, int>());
}
namespace rmf
{
    struct Map;
    class Snapshot;

    // Mutually exclusive types
    class Typed;
    class Struct;
    class Pointer;
    class Field;
    class Primitive;
    class Array;
    // End mutually exclusive types

    using NodeExclusions =
        Meta::RequireExclusive<Typed, Struct, Pointer, Field, Primitive, Array>;

    template <typename T>
    struct Missing
    {
    };
}
#endif // meta_hpp_INCLUDED
