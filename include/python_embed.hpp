#ifndef python_embed_hpp_INCLUDED
#define python_embed_hpp_INCLUDED
#include <sstream>
#include <pybind11/embed.h>

// Only for embedding python modules, the rest should be
// actual modules.

namespace py = pybind11;

struct stdoutRedirector
{
    static std::stringstream stdout;

    static void              write(py::object buffer)
    {
        stdout << buffer.cast<std::string>();
    }
    static void flush()
    {
        stdout << std::flush;
    }
};

struct stderrRedirector
{
    static std::stringstream stderr;

    static void              write(py::object buffer)
    {
        stderr << buffer.cast<std::string>();
    }
    static void flush()
    {
        stderr << std::flush;
    }
};

#endif // python_embed_hpp_INCLUDED
