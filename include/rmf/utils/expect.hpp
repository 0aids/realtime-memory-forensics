#pragma once
#include <concepts>
#include <cstddef>
#include <cstdint>

namespace rmf::Utils
{
    enum class ErrorEnum
    {
        Success = 0,
        TestError,
        MaxErrorDepthReached,
        TestBufferOverflow,
        FieldDoesNotExist,
    };

    // All Vecs and Nodes inherit from this.
    // It's just some small metadata for better graceful error handling.
    class Error
    {
      protected:
        ErrorEnum err_what = ErrorEnum::Success;

        // To be used with rmf_updErr or rmf_updRetErr.
        // Doesn't do anything with the extra information for now for minimalism.
        // Only really uses err.
        template <std::derived_from<Error> Self>
        Self& updateError(this Self& self, ErrorEnum err, const char* filename,
                          size_t line, const char* function);

      public:
        // True if error (duh!)
        bool hasError() const;
        // Returns nullptr if no error has been set.
        const char* whatError() const;
    };
}

#define rmf_retErr(exp)                                                        \
    do                                                                         \
    {                                                                          \
        if (!exp.hasError())                                                   \
            return exp;                                                        \
    } while (0)

#define rmf_updErr(exp, errc)                                                  \
    exp.updateError(errc, __FILE__, __LINE__, __FUNCTION__)

#define rmf_updRetErr(exp, errc)                                               \
    do                                                                         \
    {                                                                          \
        if (!exp.hasError())                                                   \
        {                                                                      \
            return rmf_updErr(exp, errc);                                      \
        }                                                                      \
    } while (0)

// Make an error
#define rmf_retNewErr(exp, errc)                                               \
    return exp.updateError(errc, __FILE__, __LINE__, __FUNCTION__)

namespace rmf::Utils
{
    template <std::derived_from<Error> Self>
    Self& Error::updateError(this Self& self, ErrorEnum err, const char*,
                             size_t, const char*)
    {
        self.err_what = err;
        return self;
    }
}
