#pragma once
#include <expected>
#include <string>
#include <exception>
#include <optional>
namespace RealtimeMemoryForensics::Utils
{
    enum class ErrorEnum
    {
        TestError,
        MaxErrorDepthReached,
        TestBufferOverflow,
    };

    class Error;
    using ErrE = std::unexpected<Error>;
    template <typename T>
    using ErrU = std::expected<T, Error>;

    template <typename T>
    using Opt           = std::optional<T>;
    constexpr auto nopt = std::nullopt;

    class Error : public std::exception
    {
      private:
        using lineNumber_t        = size_t;
        std::string        m_what = "";
        size_t             depth  = 0;
        static std::string generateMsg(ErrorEnum e, const char* file,
                                       lineNumber_t line,
                                       const char*  function);

        static std::string generateSubseqMsg(ErrorEnum    e,
                                             const char*  file,
                                             lineNumber_t line,
                                             const char*  function);

      public:
        Error() = default;
        Error(ErrorEnum e, const char* file, lineNumber_t line,
              const char* function);

        Error&&     update(ErrorEnum e, const char* file,
                           lineNumber_t line, const char* function);

        const char* what() const noexcept override;
        template <typename T>
        constexpr operator ErrU<T>();
    };
}

#define rmf_retErr(exp)                                              \
    do                                                               \
    {                                                                \
        if (!exp.has_value())                                        \
            return exp.error();                                      \
    } while (0)

#define rmf_updErr(exp, errc)                                        \
    exp.update(errc, __FILE__, __LINE__, __FUNCTION__)

#define rmf_updRetErr(exp, errc)                                     \
    do                                                               \
    {                                                                \
        if (!exp.has_value())                                        \
        {                                                            \
            return rmf_updErr(exp.error(), errc);                    \
        }                                                            \
    } while (0)

// Make an error
#define rmf_mkErr(errc)                                              \
    RealtimeMemoryForensics::Utils::Error(errc, __FILE__, __LINE__,  \
                                          __FUNCTION__)

namespace RealtimeMemoryForensics::Utils
{

    template <typename T>
    constexpr Error::operator ErrU<T>()
    {
        ErrU<T> errorUnion = std::unexpected<Error>(*this);
        return errorUnion;
    }
}
