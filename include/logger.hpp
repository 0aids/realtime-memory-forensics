#ifndef logger_hpp_INCLUDED
#define logger_hpp_INCLUDED

#include <iostream>
#include <sstream>
#include <mutex>
#include <string_view>
#include <thread>
enum rmf_LogLevel : uint8_t {
    rmf_Error = 0,
    rmf_Warning = 1,
    rmf_Info = 2,
    rmf_Verbose = 3,
    rmf_Debug = 4,
    rmf_Reset = 5, // For resetting colors.
};

namespace rmf {
    inline std::thread::id mainThreadId = std::this_thread::get_id();
    inline rmf_LogLevel g_logLevel = rmf_Debug;

    constexpr std::array<std::string, 6> StringColors = {
        "\033[31m", // Corresponds the the above loglevels
        "\033[33m",
        "\033[34m",
        "\033[37m",
        "\033[90m",
        "\033[0m",
    };
    constexpr std::array<std::string, 6> LogLevelNames = {
        "\033[31m[Erro]", // Corresponds the the above loglevels
        "\033[33m[Warn]",
        "\033[34m[Info]",
        "\033[37m[Verb]",
        "\033[90m[Debu]",
        "\033[0m[How?]",
    };

    class _LogWrapper {
    private:
        std::stringstream m_ss;
        static inline std::mutex logMutex;
    public:
        _LogWrapper() = default;
        _LogWrapper(const std::string_view&s);
        ~_LogWrapper();
        template <typename T>
        constexpr _LogWrapper& operator<<(const T& val)
        {
            char a;
            // Check if empty
            if (!(m_ss >> a)) return *this;
            m_ss << val;
            return *this;
        }
    };

    _LogWrapper Log(rmf_LogLevel level, 
        const std::string_view file,
        const std::string_view func,
        const size_t line);
}
#define rmf_Log(level, msg) \
{ \
     (rmf::Log(level, __FILE__, __func__, __LINE__)) << msg; \
}

#endif // logger_hpp_INCLUDED
