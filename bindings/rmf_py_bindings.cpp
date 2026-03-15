#include "logger.hpp"
#include "multi_threading.hpp"
#include "types.hpp"
#include "utils.hpp"
#include "rmf.hpp"
#include "operations.hpp"
#include <pybind11/attr.h>
#include <pybind11/gil.h>
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl_bind.h>
#include <pybind11/pytypes.h>
#include <thread>

namespace py = pybind11;

PYBIND11_MODULE(rmf_core_py, m, py::mod_gil_not_used())
{
    m.doc() = "python bindings for rmf (Realtime Memory Forensics)\n"
    		  "for use within the rmf_gui shell or externally";
}
