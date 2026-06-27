#pragma once

#include <vector>

namespace rmf
{
    template <typename N>
    concept Numeric = requires {
        std::integral<N> || std::same_as<N, double> || std::same_as<N, float>;
    };
    namespace config
    {
        template <typename T>
        using DefaultVectorLike = std::vector<T>;
    }
};
