// just a nice helper for string formatting
#include <format>

namespace RealtimeMemoryForensics::Utils
{
    template <size_t N>
    struct _fixedSizeStr
    {
        char data[N] = {};
        constexpr _fixedSizeStr(const char (&str)[N]);
    };

    template <_fixedSizeStr str>
    struct fmtLit
    {
        template <typename... Args>
        std::string fmt(Args&&... args) const;
    };
    namespace Literals
    {
        template <_fixedSizeStr str>
        constexpr auto operator""_f();
    }
}

namespace RealtimeMemoryForensics::Utils
{
    template <size_t N>
    constexpr _fixedSizeStr<N>::_fixedSizeStr(const char (&str)[N])
    {
        std::copy_n(str, N, data);
    }

    template <_fixedSizeStr str>
    template <typename... Args>
    std::string fmtLit<str>::fmt(Args&&... args) const
    {
        return std::format(std::format_string<Args...>(str.data),
                           std::forward<Args>(args)...);
    }
    namespace Literals
    {
        template <_fixedSizeStr str>
        constexpr auto operator""_f()
        {
            return fmtLit<str>{};
        }
    }
}
