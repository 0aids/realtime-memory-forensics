// Just a basic logging system.
// Has the feature of understanding repeated logging (IE every frame), and will
// update the last row that had such a thing with a count.
#pragma once

#include <array>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>

// ANSI color codes for terminal output.
const std::string RESET_COLOR  = "\033[0m";
const std::string RED_COLOR    = "\033[31m";
const std::string YELLOW_COLOR = "\033[33m";
const std::string BLUE_COLOR   = "\033[34m";
const std::string GREY_COLOR   = "\033[90m";

constexpr size_t  LOG_LENGTH = 1;

enum LogLevel {
    Error,   // Critical errors
    Warning, // Potential issues
    Message, // Key application events
    Verbose, // Detailed state information
    Debug    // Granular debugging information
};

struct LogEntry {
    std::string string;
    size_t      hash;
    size_t      count = 1;
};

// A circular array which stores the last LOG_LENGTH log entries.
class LogsList {
  private:
    std::array<LogEntry, LOG_LENGTH> m_logs;
    size_t m_index = 0; // Points to the next empty slot
    size_t m_count = 0; // Number of valid entries in the buffer

  public:
    // If a message is repeated, increments its counter; otherwise, adds it.
    void pushBack(std::string str);
    // Pushes a new message without checking for repetition.
    void pushBackNoCheck(std::string str);
};

class LoggerWrapper {
  private:
    std::optional<std::stringstream> m_ss;
    LogsList*                        m_logsList  = nullptr;
    bool                             m_checkLast = true;

  public:
    LoggerWrapper(LogsList* logsList, bool checkLast, LogLevel level,
                  std::string_view filename,
                  std::string_view functionName, size_t lineNumber);
    LoggerWrapper() = default;
    ~LoggerWrapper();

    // Stream operator to append data to the log message.
    template <typename T>
    LoggerWrapper& operator<<(const T& in) {
        if (m_ss) {
            *m_ss << in;
        }
        return *this;
    }
};

class Logger {
  private:
    static LogsList m_logsList;
    // Messages with a level higher than this will be ignored.
    constexpr static LogLevel m_logLevel = LogLevel::Debug;

  public:
    // Creates a LoggerWrapper to stream the log message into.
    static LoggerWrapper log(LogLevel         level,
                             std::string_view filename,
                             std::string_view functionName,
                             size_t lineNumber, bool checkLast);
};

// --- Logging Macros ---

// Logs a message, checking for repetition to update a counter.
#define Log(level, msg)                                              \
    {                                                                \
        (Logger::log)(level, __FILE__, __func__, __LINE__, true)     \
            << msg;                                                  \
    }

// Logs a message every time, without checking for repetition.
#define Log_f(level, msg)                                            \
    {                                                                \
        (Logger::log)(level, __FILE__, __func__, __LINE__, false)    \
            << msg;                                                  \
    }

// Logs a variable's name and its value.
#define Log_var(level, var) Log(level, #var " = " << var)
