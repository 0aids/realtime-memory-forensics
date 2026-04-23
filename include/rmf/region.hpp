#pragma once
#include <format>
#include <type_traits>

namespace RealtimeMemoryForensics
{
    // Very sus CRTP (curiosly recurring template pattern)
    template <template <typename> typename... Args>
    class Region : public Args<Region<Args...>>...
    {
        using SelfType = Region;

      public:
        operator std::string() const;
        ~Region();
    };
}

namespace RealtimeMemoryForensics
{
    template <template <typename> typename... Args>
    Region<Args...>::operator std::string() const
    {
        return (... +
                std::string(static_cast<Args<Region<Args>>>(*this)));
    }

    // Ensure that we're not being stupid.
    template <template <typename> typename... Args>
    Region<Args...>::~Region<Args...>()
    { static_assert(!std::is_polymorphic_v<SelfType>); }
}
