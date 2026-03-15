#include <gtest/gtest.h>
#include <pybind11/embed.h>
#include "python_embed.hpp"
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
