#pragma once
#include <iostream>
#include <pthread.h>
#include <print>
#include <format>

namespace RealtimeMemoryForensics::Logging
{
    namespace Detail
    {
        constexpr std::array<const char*, 7> StringColors = {
            "\033[31m", // Corresponds the the above loglevels
            "\033[33m", "\033[32m", "\033[34m",
            "\033[37m", "\033[90m", "\033[0m",
        };
        constexpr std::array<const char*, 7> LogLevelNames = {
            "Erro", // Corresponds the the above loglevels
            "Warn", " OK ", "Info", "Verb", "Debu", "How?",
        };
    }
    enum LogLevels
    {
        Error,
        Warning,
        Ok,
        Info,
        Verbose,
        Debug,
        Reset,
    };

    extern LogLevels LogLevel;
    void             setLogLevel(LogLevels level);

    std::string formatPreamble(LogLevels   level,
                               const std::string_view threadName,
                               const std::string_view filename,
                               size_t      lineNumber,
                               const std::string_view functionName);

    template <typename... Args>
    void stderrAndFmt(LogLevels level, const std::string_view filename,
                      size_t lineNumber, const std::string_view functionName,
                      std::format_string<Args...> fmtString,
                      Args&&... args)
    {
        char threadname[16] = {};
        // pthread_getname_np(pthread_self(), threadname, sizeof(threadname)) ;
        const auto preamble = formatPreamble(
            level, threadname, filename, lineNumber, functionName);
        const auto postamble =
            std::format(fmtString, std::forward<Args>(args)...);
        const auto f = std::format("{} {}{}", preamble, postamble,
               Detail::StringColors[Reset]);
        std::cerr << f << std::endl;

    }
}

#define rmf_Log(level, ...)                                          \
    {                                                                \
        if (level <= RealtimeMemoryForensics::Logging::LogLevel)     \
        {                                                            \
            RealtimeMemoryForensics::Logging::stderrAndFmt(          \
                level, __FILE__, __LINE__, __FUNCTION__,             \
                __VA_ARGS__);                                        \
        }                                                            \
    }
#define rmf_Error(...)                                               \
    rmf_Log(RealtimeMemoryForensics::Logging::Error, __VA_ARGS__);
#define rmf_Warning(...)                                             \
    rmf_Log(RealtimeMemoryForensics::Logging::Warning, __VA_ARGS__);
#define rmf_Ok(...)                                                  \
    rmf_Log(RealtimeMemoryForensics::Logging::Ok, __VA_ARGS__);
#define rmf_Info(...)                                                \
    rmf_Log(RealtimeMemoryForensics::Logging::Info, __VA_ARGS__);
#define rmf_Verbose(...)                                             \
    rmf_Log(RealtimeMemoryForensics::Logging::Verbose, __VA_ARGS__);

#define rmf_Debug(...)                                               \
    rmf_Log(RealtimeMemoryForensics::Logging::Debug, __VA_ARGS__);
