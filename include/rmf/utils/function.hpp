#ifndef functions_hpp_INCLUDED
#define functions_hpp_INCLUDED
// Custom function wrapper for threaded and mapping.
#include "rmf/utils/vec.hpp"
#include "rmf/utils/threadpool.hpp"
#include "rmf/logging/logging.hpp"
#include <functional>
#include <type_traits>

namespace RealtimeMemoryForensics::Utils
{
    template <typename T>
    struct FunctionTraitsGetter
    {
        static_assert(false,
                      "Type T is not a function, or not decoded via "
                      "a FunctionDecoder!");
    };

    template <typename R, typename... Args>
    struct FunctionTraitsGetter<std::function<R(Args...)>>
    {
        using Base        = std::function<R(Args...)>;
        using InputsTuple = std::tuple<Args...>;
        using Output      = R;
        using Signature   = R(Args...);
        using FuncPtr     = R (*)(Args...);
    };

    template <typename F>
    using FunctionDecoder =
        decltype(std::function{std::declval<F>()});

    template <typename F>
    using FuncTraits = FunctionTraitsGetter<FunctionDecoder<F>>;

    // A temporary object that holds the information
    template <typename FT>
    class Threader
    {
      private:
        FT m_mtFunc;

      public:
        using FTTraits = FuncTraits<FT>;
        Threader(FT ft);
        vec<typename FTTraits::Output> with(ThreadPool);
    };

    template <typename F, typename FT = F>
    class Function
    {
        F  m_func;
        FT m_mtFunc;

      public:
        using FTraits  = FuncTraits<F>;
        using FTTraits = FuncTraits<FT>;

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

        template <typename... Args>
        Threader<FT> threaded(Args&&... args);

        // Runs very basic parallelised (but not threaded)
        template <typename... Args>
        vec<typename FTraits::Output> applyTo(Args&&... args);
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
            std::tuple<Args...>, typename FuncTraits<F>::InputsTuple>
    typename FuncTraits<F>::Output
    Function<F, FT>::operator()(Args&&... args)
    { return m_func(std::forward<Args>(args)...); }
}
#endif // functions_hpp_INCLUDED
