#ifndef mixin_helpers_hpp_INCLUDED
#define mixin_helpers_hpp_INCLUDED

#include "rmf/utils/meta.hpp"
#include <type_traits>

// NGL don't really know what's happening here.
#define RMF_MIXIN_METHOD_PTR(methodName)                                       \
    static constexpr auto methodName##M =                                      \
        []<typename T, typename... Args>(T&& obj, Args&&... args) noexcept(    \
            noexcept(std::forward<T>(obj).methodName(                          \
                std::forward<Args>(args)...))) -> decltype(auto)               \
        requires requires {                                                    \
            std::forward<T>(obj).methodName(std::forward<Args>(args)...);      \
        }                                                                      \
    { return std::forward<T>(obj).methodName(std::forward<Args>(args)...); };  \
    static constexpr auto methodName##F = []<typename... Args>(Args&&... args) \
    {                                                                          \
        return [... args = std::move(args)](auto&& arg) mutable                \
        { return methodName##M(arg, std::forward<Args>(args)...); };           \
    }

// usage:
// template <typename T>       /*  Signature goes here!  */
// return_t MIXIN_METHOD(name, (this Self& self, args...));
#define RMF_MIXIN_METHOD(name, ...)                                            \
    name __VA_ARGS__;                                                          \
    RMF_MIXIN_METHOD_PTR(name)

#endif // mixin_helpers_hpp_INCLUDED
