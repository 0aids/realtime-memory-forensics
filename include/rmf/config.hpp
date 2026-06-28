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

    class Typed;
    class Array;
    class Struct;
    class Pointer;
    class Primitive;
    class Field;

    template <typename T>
    concept TypedCpt = requires {
        std::same_as<T, Typed> || std::same_as<T, Struct> ||
            std::same_as<T, Array> || std::same_as<T, Pointer> ||
            std::same_as<T, Primitive> || std::same_as<T, Field>;
    };
};
