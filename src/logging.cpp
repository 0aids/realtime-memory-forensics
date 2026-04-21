#include <rmf/logging/logging.hpp>
namespace mf  = RealtimeMemoryForensics;
namespace mfl = mf::Logging;

// Default log level is debug
mfl::LogLevels mfl::LogLevel = mfl::Debug;

std::string    mfl::formatPreamble(mfl::LogLevels level,
                                   const char*    threadName,
                                   const char*    filename,
                                   size_t         lineNumber,
                                   const char*    functionName)
{
    return std::format("{}[{}][{}:{} - {}]", mfl::StringColors[level],
                       mfl::LogLevelNames[level], filename,
                       lineNumber, functionName);
}
void mfl::setLogLevel(LogLevels level)
{
    mfl::LogLevel = level;
}
