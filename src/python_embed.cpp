#include "python_embed.hpp"
// Only for embedding python modules, the rest should be
// actual modules.
PYBIND11_EMBEDDED_MODULE(IoRedirector, m)
{
    py::class_<stdoutRedirector>(m, "stdoutRedirector")
        .def_static("write", &stdoutRedirector::write)
        .def_static("flush", &stdoutRedirector::flush);
    py::class_<stderrRedirector>(m, "stderrRedirector")
        .def_static("write", &stderrRedirector::write)
        .def_static("flush", &stderrRedirector::flush);
    // You have to be incredibly careful here...
    // If you don't have names correct you get a free segfault.
    m.def("hook_io",
          []()
          {
              auto py_sys = py::module::import("sys");
              auto my_sys = py::module::import("IoRedirector");
              py_sys.attr("stdout") = my_sys.attr("stdoutRedirector");
              py_sys.attr("stderr") = my_sys.attr("stderrRedirector");
          });
};

std::stringstream stdoutRedirector::stdout{};
std::stringstream stderrRedirector::stderr{};
