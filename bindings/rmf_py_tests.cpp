#include <pybind11/pybind11.h>

namespace py = pybind11;

PYBIND11_MODULE(rmf_core_py, m, py::mod_gil_not_used())
{
    m.doc() = "Helpers for testing for rmf_py";
}
