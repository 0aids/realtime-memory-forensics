#pragma once
#include <functional>
#include <memory>
#include <vector>
namespace RealtimeMemoryForensics::Utils
{
    template <typename T, typename Allocator = std::allocator<T>>
    class vec : public std::vector<T, Allocator>
    {
      private:
      public:
        using BaseType = std::vector<T, Allocator>;
        using BaseType::BaseType;

        template <typename T1, typename... Args>
        vec<T1> map(const std::function<T1(const T&, Args&&...)>,
                    Args&&... args);

        template <typename... Args>
        BaseType
        filter(const std::function<bool(const T&, Args&&...)>,
               Args&&... args);
    };
}
