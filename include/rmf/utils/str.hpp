// just a nice helper for string formatting
#include <format>

namespace RealtimeMemoryForensics::Utils
{
    namespace Detail
    {

        template <size_t N>
        struct FixedSizeStr
        {
            char data[N] = {};
            constexpr FixedSizeStr(const char (&str)[N]);
        };

        template <FixedSizeStr str>
        struct FormatLiteral
        {
            template <typename... Args>
            std::string fmt(Args&&... args) const;
        };
    }
    namespace Literals
    {
        template <Detail::FixedSizeStr str>
        constexpr auto operator""_f();
    }
}

// Implementation
namespace RealtimeMemoryForensics::Utils
{
    template <size_t N>
    constexpr Detail::FixedSizeStr<N>::FixedSizeStr(
        const char (&str)[N])
    { std::copy_n(str, N, data); }

    template <Detail::FixedSizeStr str>
    template <typename... Args>
    std::string Detail::FormatLiteral<str>::fmt(Args&&... args) const
    {
        return std::format(std::format_string<Args...>(str.data),
                           std::forward<Args>(args)...);
    }

    namespace Literals
    {
        template <Detail::FixedSizeStr str>
        constexpr auto operator""_f()
        { return Detail::FormatLiteral<str>{}; }
    }
}
