#ifndef executor_hpp_INCLUDED
#define executor_hpp_INCLUDED
#include <utility>
#include <functional>
#include "logs.hpp"
#include "concepts.hpp"
#include "threadpool.hpp"

template <typename R, typename T>
concept ReturnsTOrRefT = std::same_as<R, T> || std::same_as<R, T&>;

template <typename Buildable>
concept BuildableAndRegionProperties =
    requires(decltype(Buildable::Builder::regionProperties) rp) {
        { rp } -> ReturnsTOrRefT<MemoryRegionProperties>;
    };

template <typename CoreFuncType, Buildable OutputType,
          typename... CoreInputs, typename... BuildInputs>
    requires Buildable<OutputType>
// 1 to 1
// requires CoreFuncConcept<CoreFuncType, OutputType, BuildInputs...>
OutputType makeGenericST(BuildJob<OutputType>& job,
                         MemoryPartition part, CoreFuncType coreFunc,
                         CoreInputs... coreInputs)
{
    // Solved by decoupling partitioning -- We do not need to
    // partition by ourselves. We make the partitions beforehand, outside
    // of the function.
    // auto part =
    //     makeMemoryPartitions(job.build.regionProperties, 1).front();
    coreFunc(job, part, std::forward<CoreInputs>(coreInputs)...);
    return job.getResult();
}

template <typename CoreFuncType, Buildable OutputType,
          typename... CoreInputs, typename... BuildInputs>
    requires Buildable<OutputType>
// N to 1
OutputType makeGenericMT(ThreadPool& tp, BuildJob<OutputType>& job,
                         std::vector<MemoryPartition> parts,
                         CoreFuncType                 coreFunc,
                         CoreInputs... coreInputs)
{
    for (auto& part : parts)
    {
        // Job outlives this function, and the tasked threads.
        // coreFunc outlives the loop
        // coreInputs outlives the loop
        tp.submitTask(
            [&job, part, &coreFunc, &coreInputs...]() -> void
            {
                coreFunc(job, part,
                         std::forward<CoreInputs>(coreInputs)...);
            });
    }

    tp.joinTasks();
    return job.getResult();
}

/*
 * A generic function that runs core functions, mainly ones that have an
 * unknown amount of outputs (IE a vector of MemoryRegionProperties), but
 * supports multithreading. Creates multiple builds, multithreads them,
 * and then consolidates them.
 * */
template <typename CoreFuncType,
          BuildableAndConsolidatable OutputType,
          typename... CoreInputs, typename... BuildInputs>
    requires Buildable<OutputType> && Consolidatable<OutputType>
// ? to ? to 1 (Last conversion is not on main thread).
OutputType
makeGenericConsolidateMT(ThreadPool& tp, ConsolidateJob<OutputType> consolidator,
                         std::vector<MemoryPartition> parts,
                         CoreFuncType                 coreFunc,
                         CoreInputs&... coreInputs)
{
    for (size_t i = 0; i < consolidator.consolidator.builders.size(); i++) 
    {
        tp.submitTask([
                      &builder = consolidator.consolidator.builders[i],
                      &part = parts[i],
                      coreFunc,
                      &coreInputs...]
              ()
              {
                  coreFunc(builder, part, std::forward<CoreInputs>(coreInputs)...);
              });
    }
    tp.joinTasks();

    return consolidator.consolidate();
}

// Reference for a buildable and or consolidatable structure

template <typename... BuilderArgs>
struct OutputType {
    struct Builder {
        // The main purpose of the builder is to hold temporary data for
        // const correctness, as well as provide a standardised way for
        // separation of concerns and responsibilities (threading mainly)

        int intdata;
        double doubledata;
        std::vector<int> intvectordata;
        // And so on, whatever is needed to create the OutputType


        // For some certain things, you would ensure that
        // a certain vector is resized correctly (for threading).
        // This is done by giving the builder some arguments.
        Builder(BuilderArgs... args);

        // Complete the type.
        OutputType build() {
            // Some shit goes here.
            return {};
        };
    };

    template <typename... ConsolidatorArgs>
    struct Consolidator {
        // The main purpose of the consolidator is to consolidate multiple builds
        // required by core functions that are special - They have an unknown amount
        // of return values in a vector.
        std::vector<OutputType::Builder> builders;
        OutputType consolidate();

        // The constructor can take in args that it uses to build the builders.
        Consolidator(ConsolidatorArgs... args);

    };
};


#endif // executor_hpp_INCLUDED
