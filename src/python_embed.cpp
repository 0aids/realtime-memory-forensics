#include "python_embed.hpp"
#include "logger.hpp"
#include <pybind11/embed.h>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>

namespace py = pybind11;

rmf::py::embedPythonScopedGuard::embedPythonScopedGuard()
    :m_interpreterGuard(), m_locals()
{
    py::exec(R"(
        class Redirector:
            def __init__(self, writeFunc):
            	self.writeFunc = writeFunc
            def write(self, s):
            	self.writeFunc(s)
            def flush(self):
                pass
    )", py::globals(), m_locals);
    py::object Redirector = m_locals["Redirector"];
    py::cpp_function stdoutWrite([this] (const std::string& s)
    {
        m_stdoutBuffer << s;
    });
    py::cpp_function stderrWrite([this] (const std::string& s)
    {
        m_stderrBuffer << s;
    });

    py::object stdoutRedir = Redirector(stdoutWrite);
    py::object stderrRedir = Redirector(stderrWrite);

    py::module sys = py::module::import("sys");
    m_oldStderr = sys.attr("stderr");
    m_oldStdout = sys.attr("stdout");

    sys.attr("stderr") = stderrRedir;
    sys.attr("stdout") = stdoutRedir;

    // Import our libraries.
    sys.attr("path").attr("append")("../rmf_py");

	try {
        py::exec(R"(
            import rmf_py as rmf
        )");
    }
    catch (const py::error_already_set& e)
    {
        rmf_Log(rmf_Error, "Failed to import rmf_py, error: " << e.what());
    }
}

rmf::py::embedPythonScopedGuard::~embedPythonScopedGuard()
{
    py::module sys = py::module::import("sys");
    sys.attr("stderr") = m_oldStderr;
    sys.attr("stdout") = m_oldStdout;
}

std::string rmf::py::embedPythonScopedGuard::getStderr() const
{
    return m_stderrBuffer.str();
}

std::string rmf::py::embedPythonScopedGuard::getStdout() const
{
    return m_stdoutBuffer.str();
}
