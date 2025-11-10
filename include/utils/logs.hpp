#ifndef logs_hpp_INCLUDED
#define logs_hpp_INCLUDED
// All printed info, debug and logs are in stderr

#include <thread>
#include <pthread.h>
#include <iostream>
#include <optional>
#include <sstream>
#include <mutex>
enum rmf_LogLevel
{
    rmf_Error   = 0, // Critical errors
    rmf_Warning = 1, // Potential issues
    rmf_Message = 2, // Key application events
    rmf_Verbose = 3, // Detailed state information
    rmf_Debug   = 4, // Granular debugging information
};

namespace rmf::utils
{
inline std::mutex logMutex;
inline std::thread::id mainThreadID = std::this_thread::get_id();

// ANSI color codes for terminal output.
constexpr std::string RESET_COLOR  = "\033[0m";
constexpr std::string RED_COLOR    = "\033[31m";
constexpr std::string YELLOW_COLOR = "\033[33m";
constexpr std::string BLUE_COLOR   = "\033[34m";
constexpr std::string GREY_COLOR   = "\033[90m";


class LoggerWrapper
{
  private:
    std::optional<std::stringstream> m_ss;

  public:
    template <typename T>
    constexpr LoggerWrapper& operator<<(const T& in)
    {
        if (m_ss)
        {
            *m_ss << in;
        }
        return *this;
    }

    constexpr LoggerWrapper() = default;

    constexpr LoggerWrapper(const std::string_view initial)
    {
        m_ss.emplace();
        *m_ss << initial;
    }

    constexpr ~LoggerWrapper()
    {
        if (m_ss)
        {
            // Prevent funky shit happening when printing.
            std::lock_guard<std::mutex> lock(logMutex);
            std::cerr << m_ss.value().str() << RESET_COLOR
                      << "\n"; // stop blocking.
        }
    }
};

class Logger
{
  private:
    static
        // Helper to convert LogLevel enum to a colored string representation.
        std::string
        levelToString(rmf_LogLevel level)
    {
        switch (level)
        {
            case rmf_LogLevel::rmf_Error: return RED_COLOR + "[ERROR]  ";
            case rmf_LogLevel::rmf_Warning: return YELLOW_COLOR + "[WARNING]";
            case rmf_LogLevel::rmf_Message: return BLUE_COLOR + "[MESSAGE]";
            case rmf_LogLevel::rmf_Verbose: return "[VERBOSE]";
            case rmf_LogLevel::rmf_Debug: return GREY_COLOR + "[DEBUG]  ";
            default: return "[UNKNOWN]";
        }
    }

  public:
    static inline rmf_LogLevel currentLevel = rmf_Debug;
    constexpr static LoggerWrapper   log(const rmf_LogLevel         level,
                               const std::string_view filename,
                               const std::string_view functionName,
                               size_t                 lineNumber)
    {

        if (level > currentLevel)
            return {};
        // TODO: Add shit for detecting which thread is currently printing.
        std::stringstream init;

        auto threadID = std::this_thread::get_id();

        if (threadID != mainThreadID) {
            char name[20]{};
            pthread_getname_np(pthread_self(), name, sizeof(name));
            init << "[T"
                 << name
                 << "]\t";
        } else {
            init << "[TM]\t";
        }

        init  << levelToString(level) << " (" << filename << ":"
             << functionName << ":" << lineNumber << ") ";
        return LoggerWrapper(init.str());
        ;
    }
};
}

// Logs a message, checking for repetition to update a counter.
#define rmf_Log(level, msg)                                              \
    {                                                                \
        (rmf::utils::Logger::log)(level, __FILE__, __func__, __LINE__) << msg;   \
    }

// Logs a variable's name and its value.
#define rmf_Log_var(level, var) Log(level, #var " = " << var)

#endif // logs_hpp_INCLUDED
