#include "python_embed.hpp"
#include "logger.hpp"
#include <pybind11/embed.h>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>

namespace py = pybind11;

rmf::py::embedPythonScopedGuard::embedPythonScopedGuard(
    RedirectPolicy policy) : m_interpreterGuard(), m_locals()
{
    py::exec(R"(
        class Redirector:
            def __init__(self, writeFunc):
            	self.writeFunc = writeFunc
            def write(self, s):
            	self.writeFunc(s)
            def flush(self):
                pass
    )",
             py::globals(), m_locals);
    py::object       Redirector = m_locals["Redirector"];
    py::cpp_function stdoutWrite([this](const std::string& s)
                                 { m_stdoutBuffer << s; });
    py::cpp_function stderrWrite([this](const std::string& s)
                                 { m_stderrBuffer << s; });

    py::object       stdoutRedir = Redirector(stdoutWrite);
    py::object       stderrRedir = Redirector(stderrWrite);

    py::module       sys = py::module::import("sys");
    if (policy == RedirectPolicy::All ||
        policy == RedirectPolicy::Stderr)
    {
        m_oldStderr        = sys.attr("stderr");
        sys.attr("stderr") = stderrRedir;
    }
    if (policy == RedirectPolicy::All ||
        policy == RedirectPolicy::Stdout)
    {
        m_oldStdout        = sys.attr("stdout");
        sys.attr("stdout") = stdoutRedir;
    }

    // Import our libraries.
    sys.attr("path").attr("append")("../rmf_py");

    try
    {
        py::exec(R"(
        import rmf_py as rmf
        import faulthandler; faulthandler.enable()
        )",
                 py::globals(), m_locals);
    }
    catch (const py::error_already_set& e)
    {
        rmf_Log(rmf_Error,
                "Failed to import rmf_py, error: " << e.what());
    }
}

rmf::py::embedPythonScopedGuard::~embedPythonScopedGuard()
{
    rmf_Log(rmf_Info, "stdout: \n" << m_stdoutBuffer.str());
    rmf_Log(rmf_Warning, "stderr: \n" << m_stderrBuffer.str());
    py::module sys     = py::module::import("sys");
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

py::dict rmf::py::embedPythonScopedGuard::getGlobals()
{
    return py::globals();
}

py::dict rmf::py::embedPythonScopedGuard::getLocals()
{
    return m_locals;
}

void rmf::py::embedPythonScopedGuard::clearStderr()
{
    m_stderrBuffer.clear();
}
void rmf::py::embedPythonScopedGuard::clearStdout()
{
    m_stdoutBuffer.clear();
}

bool rmf::py::embedPythonScopedGuard::execString(
    const std::string_view& view)
{
    rmf_Log(rmf_Info,
            "Executing the following python code: \n"
                << view);
    try
    {
        py::exec(view, py::globals(), m_locals);
        return true;
    }
    catch (const py::error_already_set& e)
    {
        rmf_Log(rmf_Error,
                "Failed to execute python, see interpreter's stderr");
        m_stderrBuffer << e.what();
        return false;
    }
}
