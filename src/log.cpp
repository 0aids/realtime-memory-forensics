#include "log.hpp"

#include <iostream>
#include <string>
#include <utility> // For std::move

// Mainly written by me, tidied up by ai cause lazy.

namespace {
    // Helper to convert LogLevel enum to a colored string representation.
    std::string levelToString(LogLevel level) {
        switch (level) {
            case LogLevel::Error: return RED_COLOR + "[ERROR]  ";
            case LogLevel::Warning: return YELLOW_COLOR + "[WARNING]";
            case LogLevel::Message: return BLUE_COLOR + "[MESSAGE]";
            case LogLevel::Verbose: return "[VERBOSE]";
            case LogLevel::Debug: return GREY_COLOR + "[DEBUG]  ";
            default: return "[UNKNOWN]";
        }
    }
} // namespace

// --- LoggerWrapper Implementation ---

LoggerWrapper::LoggerWrapper(LogsList* logsList, bool checkLast,
                             LogLevel         level,
                             std::string_view filename,
                             std::string_view functionName,
                             size_t           lineNumber) :
    m_logsList(logsList), m_checkLast(checkLast) {
    if (m_logsList) {
        m_ss.emplace(); // Construct the std::stringstream
        // Prepend the log message with level, file, and function info.
        *m_ss << levelToString(level) << " (" << filename << ":"
              << functionName << ":" << lineNumber << ") ";
    }
}

LoggerWrapper::~LoggerWrapper() {
    if (!m_ss) {
        return;
    }

    if (m_checkLast) {
        m_logsList->pushBack(std::move(m_ss->str()));
    } else {
        m_logsList->pushBackNoCheck(std::move(m_ss->str()));
    }
}

// --- Logger Implementation ---

// Static member initialization.
LogsList      Logger::m_logsList;

LoggerWrapper Logger::log(LogLevel level, std::string_view filename,
                          std::string_view functionName,
                          size_t lineNumber, bool checkLast) {
    if (level > m_logLevel) {
        return LoggerWrapper(); // Return an empty wrapper that does nothing.
    }

    return LoggerWrapper(&m_logsList, checkLast, level, filename,
                         functionName, lineNumber);
}

// --- LogsList Implementation ---

void LogsList::pushBack(std::string str) {
    const std::hash<std::string> hasher;
    const size_t                 hash = hasher(str);

    // Search backwards from the most recent log entry to find a match.
    for (size_t k = 1; k <= m_count; ++k) {
        const size_t j = (m_index - k + LOG_LENGTH) % LOG_LENGTH;
        if (m_logs[j].hash == hash) {
            m_logs[j].count++;
            // Use ANSI codes to move cursor up, clear line, and reprint message.
            const size_t linesToMoveUp = k;
            std::cerr << "\033[s"; // Save cursor position
            std::cerr << "\033[" << linesToMoveUp
                      << "A";         // Move cursor up
            std::cerr << "\033[2K\r"; // Clear line and return
            std::cerr << m_logs[j].string
                      << " (count: " << m_logs[j].count << ")"
                      << RESET_COLOR;
            std::cerr << "\033[u"; // Restore cursor position
            std::cerr.flush();
            return;
        }
    }

    // No match found, so add a new log entry.
    m_logs[m_index] = LogEntry{std::move(str), hash, 1};
    std::cerr << m_logs[m_index].string << RESET_COLOR << "\n";
    m_index = (m_index + 1) % LOG_LENGTH;
    if (m_count < LOG_LENGTH) {
        m_count++;
    }
}

void LogsList::pushBackNoCheck(std::string str) {
    const std::hash<std::string> hasher;
    const size_t                 hash = hasher(str);

    m_logs[m_index] = LogEntry{std::move(str), hash, 1};
    std::cerr << m_logs[m_index].string << "\n";
    m_index = (m_index + 1) % LOG_LENGTH;
    if (m_count < LOG_LENGTH) {
        m_count++;
    }
}
