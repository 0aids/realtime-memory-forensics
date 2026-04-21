#pragma once
#include <print>
#include <iostream>
#include <pthread.h>
#include <string_view>
#include <thread>

namespace RealtimeMemoryForensics::Logging
{
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

    constexpr std::array<std::string, 7> StringColors = {
        "\033[31m", // Corresponds the the above loglevels
        "\033[33m", "\033[32m", "\033[34m",
        "\033[37m", "\033[90m", "\033[0m",
    };
    constexpr std::array<std::string, 7> LogLevelNames = {
        "Erro", // Corresponds the the above loglevels
        "Warn", " OK ", "Info", "Verb", "Debu", "How?",
    };

    extern LogLevels LogLevel;
    void             setLogLevel(LogLevels level);

    template <typename... Args>
    void stdout(std::format_string<Args...> fmtString, Args&&... args)
    {
        println(fmtString, args...);
    }

    template <typename... Args>
    void stderr(std::format_string<Args...> fmtString, Args&&... args)
    {
        println(std::cerr, fmtString, args...);
    }

    std::string formatPreamble(LogLevels   level,
                               const char* threadName,
                               const char* filename,
                               size_t      lineNumber,
                               const char* functionName);

    template <typename... Args>
    void stderrAndFmt(LogLevels level, const char* filename,
                      size_t lineNumber, const char* functionName,
                      std::format_string<Args...> fmtString,
                      Args&&... args)
    {
        char threadname[16] = {};
        // pthread_getname_np(pthread_self(), threadname, sizeof(threadname)) ;
        const auto preamble = formatPreamble(
            level, threadname, filename, lineNumber, functionName);
        const auto postamble = std::format(fmtString, args...);
        stderr("{} {}", preamble, postamble);
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
