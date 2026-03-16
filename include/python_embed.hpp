#ifndef python_embed_hpp_INCLUDED
#define python_embed_hpp_INCLUDED
#include <pybind11/pytypes.h>
#include <sstream>
#include <pybind11/embed.h>
#include <string_view>

// Only for embedding python modules, the rest should be
// actual modules.


namespace rmf::py
{
    namespace py = pybind11;

    class embedPythonScopedGuard
    {
    private:
        py::scoped_interpreter m_interpreterGuard;
        std::stringstream m_stdoutBuffer;
        std::stringstream m_stderrBuffer;

        py::object m_oldStdout;
        py::object m_oldStderr;
        py::dict m_locals;

    public:
        embedPythonScopedGuard();
        ~embedPythonScopedGuard();
        embedPythonScopedGuard(embedPythonScopedGuard&&) = delete;
        embedPythonScopedGuard(const embedPythonScopedGuard&) = delete;
        embedPythonScopedGuard& operator=(embedPythonScopedGuard&&) = delete;
        embedPythonScopedGuard& operator=(const embedPythonScopedGuard&) = delete;

        std::string getStdout() const;
        std::string getStderr() const;
        void clearStderr();
        void clearStdout();
        py::dict getGlobals();
        py::dict getLocals();

		// True for successful execution, false otherwise
        bool execString(const std::string_view& view);
    };
}


#endif // python_embed_hpp_INCLUDED
