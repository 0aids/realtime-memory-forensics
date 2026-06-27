#pragma once

#include <type_traits>
#include <vector>

namespace rmf
{
    template <typename N>
    concept Numeric = requires { std::is_arithmetic_v<N>; };
    namespace config
    {
        template <typename T>
        using DefaultVectorLike = std::vector<T>;
    }
};
