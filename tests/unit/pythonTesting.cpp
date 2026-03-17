#include <gtest/gtest.h>
#include <pybind11/embed.h>
#include "test_helpers.hpp"
#include "logger.hpp"
#include "python_embed.hpp"
#include "types.hpp"
#include <sstream>
#include <iostream>
namespace py = pybind11;

TEST(initialPythonTests, helloWorld)
{
    rmf::py::embedPythonScopedGuard guard{};

    py::exec(R"(print("Hello world!"))");

    EXPECT_STREQ(guard.getStdout().c_str(), "Hello world!\n");
    std::cout << guard.getStdout().c_str() << std::endl;
}

TEST(initialPythonTests, parseMaps)
{
    using namespace std::literals;
    rmf::types::MemoryRegionPropertiesVec results{};
    rmf::test::testProcess                tp;
    tp.build<rmf::test::staticValueComponent>();
    pid_t       pid = tp.run();
    std::string stdoutBuffer;
    std::string stderrBuffer;
    {
        rmf::py::embedPythonScopedGuard guard(
            rmf::py::RedirectPolicy::All);

        if (guard.execString("maps = rmf.getMapsFromPid(" +
                             std::to_string(pid) + ")"))
        {
            results =
                guard.getLocals()["maps"]
                    .cast<rmf::types::MemoryRegionPropertiesVec>();
        }
    }
    tp.stop();

    rmf_Log(rmf_Info, "Number of results found: " << results.size());
    EXPECT_GT(results.size(), 0);
}
