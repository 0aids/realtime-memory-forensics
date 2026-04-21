#include <rmf/logging/logging.hpp>
namespace mf   = RealtimeMemoryForensics;
namespace mfl  = mf::Logging;
namespace mfld = mf::Logging::Detail;

// Default log level is debug
mfl::LogLevels mfl::LogLevel = mfl::Debug;

std::string    mfl::formatPreamble(mfl::LogLevels level,
                                   const char*    threadName,
                                   const char*    filename,
                                   size_t         lineNumber,
                                   const char*    functionName)
{
    return std::format("{}[{}][{}:{} - {}]",
                       mfld::StringColors[level],
                       mfld::LogLevelNames[level], filename,
                       lineNumber, functionName);
}
void mfl::setLogLevel(LogLevels level)
{ mfl::LogLevel = level; }
