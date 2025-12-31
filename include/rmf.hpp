#ifndef rmf_hpp_INCLUDED
#define rmf_hpp_INCLUDED
// Holds the main API calls.
#include "multi_threading.hpp"
#include "types.hpp"
#include "utils.hpp"
#include <ranges>
#include <tuple>
#include <type_traits>

namespace rmf
{
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

      public:
        Analyzer(size_t numThreads);
        std::shared_ptr<Impl> getImpl() {
            return m_impl;
        }
        template <typename func_t, typename... Args>
        auto Execute(func_t func, Args... args)
        {
            using FuncType = decltype(std::function{func});
            using Return_t = FuncType::result_type;
            // Ewwwww
            // Search up pack expressions
            // Essentially allows me to apply functions to a variadic template in a constexpr manner
            std::vector<Return_t> results;
            size_t vecSize = 1;
            // Get vector size.
            (
                [&]() mutable
                {
                    if constexpr (utils::IsContainer<decltype(args)>)
                    {
                        vecSize = args.size();
                    }
                }(),
                ...);
            results.reserve(vecSize);
            std::vector<Task_t<Return_t>> tasksList;

            for (size_t i = 0; i < vecSize; i++)
            {
                // IDK if i like this or not...
                auto tuple = std::make_tuple(func, 
                    [&]() mutable
                    {
                        if constexpr (utils::IsContainer<decltype(args)>)
                        {
                            return args[i];
                        }
                        else // NOTE: It's your fault if it's slow, we copy anyways. Hope that what ur copying is trivially copyable.
                        {
                            return args;
                        }
                    }()...
                );
                // utils::typePrinter<decltype(tuple)> b;
                tasksList.push_back(std::make_from_tuple<Task_t<Return_t>>(tuple));
                m_impl->tp.SubmitTask(tasksList.back());
            }

            for (auto &task:tasksList) {
                results.push_back(task.getFuture().get());
            }
            return results;
        }
    };
}

#endif // rmf_hpp_INCLUDED
