#include <gtest/gtest.h>
#include <pybind11/embed.h>
#include "python_embed.hpp"
#include <sstream>
#include <iostream>
namespace py = pybind11;

TEST(initialPythonTests, helloWorld)
{
    {
        py::scoped_interpreter guard{};
        py::module::import("IoRedirector").attr("hook_io")();

        py::exec(R"(print("Hello world!"))");

        EXPECT_STREQ(stdoutRedirector::stdout.str().c_str(), "Hello world!\n");
        std::cout << stdoutRedirector::stdout.str() << std::endl;
    }
}
