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
    enum class RedirectPolicy
    {
        None,
        Stdout,
        Stderr,
        Both,
        All,
    };

    class embedPythonScopedGuard
    {
      private:
        py::scoped_interpreter m_interpreterGuard;
        std::stringstream      m_stdoutBuffer;
        std::stringstream      m_stderrBuffer;

        py::object             m_oldStdout;
        py::object             m_oldStderr;

      public:
        embedPythonScopedGuard(
            RedirectPolicy policy = RedirectPolicy::Stdout);
        ~embedPythonScopedGuard();
        embedPythonScopedGuard(embedPythonScopedGuard&&) = delete;
        embedPythonScopedGuard(const embedPythonScopedGuard&) =
            delete;
        embedPythonScopedGuard&
        operator=(embedPythonScopedGuard&&) = delete;
        embedPythonScopedGuard&
                    operator=(const embedPythonScopedGuard&) = delete;

        std::string getStdout() const;
        std::string getStderr() const;
        void        clearStderr();
        void        clearStdout();
        py::dict    getGlobals();

        // True for successful execution, false otherwise
        bool execString(const std::string_view& view);
    };
}

#endif // python_embed_hpp_INCLUDED
