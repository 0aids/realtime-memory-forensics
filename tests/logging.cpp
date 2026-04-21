#include <gtest/gtest.h>
#include <rmf/logging/logging.hpp>
#include <rmf/rmf.hpp>

namespace mf  = RealtimeMemoryForensics;
namespace mfl = mf::Logging;

TEST(logging, AttemptLogging)
{
    rmf_Error("Error Test");
    rmf_Warning("Warning Test");
    rmf_Info("Info Test");
    rmf_Ok("Ok Test");
    rmf_Verbose("Verbose Test");
    rmf_Debug("Debug Test");
}

TEST(logging, AttemptLoggingErrorOnly)
{
    mfl::setLogLevel(mfl::Error);
    rmf_Error("Error Test");
    rmf_Warning("Warning Test");
    rmf_Info("Info Test");
    rmf_Ok("Ok Test");
    rmf_Verbose("Verbose Test");
    rmf_Debug("Debug Test");
}

TEST(Logging, AttemptLoggingVariables)
{
    int a = 0;
    int b = -10;
    rmf_Debug("a - b = {}", a - b);
}
