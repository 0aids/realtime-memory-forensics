#include <rmf/logging/logging.hpp>
#include <string_view>
namespace mf   = RealtimeMemoryForensics;
namespace mfl  = mf::Logging;
namespace mfld = mf::Logging::Detail;

// Default log level is debug
mfl::LogLevels mfl::LogLevel = mfl::Debug;

std::string    mfl::formatPreamble(mfl::LogLevels         level,
                                   const std::string_view threadName,
                                   const std::string_view filename,
                                   size_t                 lineNumber,
                                   const std::string_view functionName)
{
    return std::format("{}[{}][{}:{} - {}]",
                       mfld::StringColors[level],
                       mfld::LogLevelNames[level], filename,
                       lineNumber, functionName);
}
void mfl::setLogLevel(LogLevels level)
{
    mfl::LogLevel = level;
}
