#include <gtest/gtest.h>
#include <rmf/logging/logging.hpp>
#include <rmf/rmf.hpp>
#include <sstream>

namespace mf  = RealtimeMemoryForensics;
namespace mfl = mf::Logging;

TEST(Logging, AttemptLogging)
{
    rmf_Error("Error Test");
    rmf_Warning("Warning Test");
    rmf_Info("Info Test");
    rmf_Ok("Ok Test");
    rmf_Verbose("Verbose Test");
    rmf_Debug("Debug Test");
}

TEST(Logging, AttemptLoggingErrorOnly)
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

TEST(Logging, setLogLevel_filtersBelowLevel)
{
    mfl::setLogLevel(mfl::Warning);
    rmf_Error("error");
    rmf_Warning("warning");
    rmf_Info("info");
    rmf_Ok("ok");
    rmf_Verbose("verbose");
    rmf_Debug("debug");
}

TEST(Logging, setLogLevel_errorOnly)
{
    mfl::setLogLevel(mfl::Error);
    rmf_Error("error");
    rmf_Warning("warning");
    rmf_Info("info");
}

TEST(Logging, setLogLevel_debugShowsAll)
{
    mfl::setLogLevel(mfl::Debug);
    rmf_Error("error");
    rmf_Warning("warning");
    rmf_Info("info");
    rmf_Ok("ok");
    rmf_Verbose("verbose");
    rmf_Debug("debug");
}

TEST(Logging, sequentialLogging)
{
    for (int i = 0; i < 3; i++)
    {
        rmf_Info("message {}", i);
    }
}

TEST(Logging, logWithIntFormat)
{
    rmf_Info("count={}", 42);
    rmf_Info("sum={}", 1 + 2 + 3);
}

TEST(Logging, logWithStringFormat)
{
    rmf_Info("hello {}", "world");
    rmf_Info("{} {}", "a", "b");
}

TEST(Logging, logWithFloatFormat)
{ rmf_Info("pi={}", 3.14); }
