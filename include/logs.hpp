#ifndef logs_hpp_INCLUDED
#define logs_hpp_INCLUDED
// All printed info, debug and logs are in stderr

#include <thread>
#include <pthread.h>
#include <iostream>
#include <optional>
#include <sstream>
#include <mutex>

inline std::mutex logMutex;
inline std::thread::id mainThreadID = std::this_thread::get_id();

// ANSI color codes for terminal output.
constexpr std::string RESET_COLOR  = "\033[0m";
constexpr std::string RED_COLOR    = "\033[31m";
constexpr std::string YELLOW_COLOR = "\033[33m";
constexpr std::string BLUE_COLOR   = "\033[34m";
constexpr std::string GREY_COLOR   = "\033[90m";

enum LogLevel
{
    Error   = 0, // Critical errors
    Warning = 1, // Potential issues
    Message = 2, // Key application events
    Verbose = 3, // Detailed state information
    Debug   = 4, // Granular debugging information
};

class LoggerWrapper
{
  private:
    std::optional<std::stringstream> m_ss;
    LogLevel                         level;

  public:
    template <typename T>
    LoggerWrapper& operator<<(const T& in)
    {
        if (m_ss)
        {
            *m_ss << in;
        }
        return *this;
    }

    LoggerWrapper() = default;

    LoggerWrapper(const std::string_view initial)
    {
        m_ss.emplace();
        *m_ss << initial;
    }

    ~LoggerWrapper()
    {
        if (m_ss)
        {
            // Prevent funky shit happening when printing.
            std::lock_guard<std::mutex> lock(logMutex);
            std::cerr << m_ss.value().str() << RESET_COLOR
                      << std::endl;
        }
    }
};

class Logger
{
  private:
    static
        // Helper to convert LogLevel enum to a colored string representation.
        std::string
        levelToString(LogLevel level)
    {
        switch (level)
        {
            case LogLevel::Error: return RED_COLOR + "[ERROR]  ";
            case LogLevel::Warning: return YELLOW_COLOR + "[WARNING]";
            case LogLevel::Message: return BLUE_COLOR + "[MESSAGE]";
            case LogLevel::Verbose: return "[VERBOSE]";
            case LogLevel::Debug: return GREY_COLOR + "[DEBUG]  ";
            default: return "[UNKNOWN]";
        }
    }

  public:
    static inline LogLevel currentLevel = Debug;
    static LoggerWrapper   log(const LogLevel         level,
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

// Logs a message, checking for repetition to update a counter.
#define Log(level, msg)                                              \
    {                                                                \
        (Logger::log)(level, __FILE__, __func__, __LINE__) << msg;   \
    }

// Logs a variable's name and its value.
#define Log_var(level, var) Log(level, #var " = " << var)

#endif // logs_hpp_INCLUDED
