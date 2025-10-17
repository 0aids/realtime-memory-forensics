#ifndef concepts_hpp_INCLUDED
#define concepts_hpp_INCLUDED

#include <vector>
#include <concepts>
#include <utility>

// TODO: Fix the CoreFunc concept? Concepts seem useful.
// template <typename F, typename OutputType, typename... Args>
// concept CoreFuncConcept =
//     requires(F f, BuildJob<OutputType> job, Args&&... args) {
//         { f(job, std::forward(args)...) } -> std::same_as<void>;
//     };
//     

// I did not know this feature existed.
// Although this is some abominable syntax

// Below means the following:
// The concept Buildable is a class that has a subclass T::Builder
// And for any T::Builder builder, it implements a function T::Builder::build, that returns
// a type that is the same as T or const T.

template <typename R, typename T>
concept ReturnsTOrConstT =
    std::same_as<R, T> || std::same_as<R, const T>;

template <typename T, typename... Args>
concept Buildable = requires(T::Builder builder, Args&&... args) {
    {
        builder.build(std::forward<Args>(args)...)
    } -> ReturnsTOrConstT<T>;
};

template <typename OutputType>
concept Consolidatable = requires(
    OutputType::Consolidator consolidater
)
{
    {
        consolidater.consolidate()
    } -> ReturnsTOrConstT<OutputType>;
} 
// && std::same_as<typename OutputType::Consolidator::builders, std::vector<typename OutputType::Builder>>
;

template <typename OutputType>
concept BuildableAndConsolidatable = Consolidatable<OutputType> && Buildable<OutputType>;

#endif // concepts_hpp_INCLUDED
