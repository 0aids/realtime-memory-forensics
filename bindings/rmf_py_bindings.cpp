#include "types.hpp"
#include "utils.hpp"
#include <pybind11/attr.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl_bind.h>

namespace py = pybind11;

PYBIND11_MAKE_OPAQUE(std::vector<rmf::types::MemoryRegionProperties>);

PYBIND11_MODULE(rmf_py, m, py::mod_gil_not_used()) {
    m.doc() = "Realtime Memory Forensics - Python Bindings";
    py::enum_<rmf::types::Perms>(m, "Perms", py::module_local())
        .value("none", rmf::types::Perms::None)
        .value("read", rmf::types::Perms::Read)
        .value("write", rmf::types::Perms::Write)
        .value("execute", rmf::types::Perms::Execute)
        .value("shared", rmf::types::Perms::Shared)
        ;

    py::class_<rmf::types::MemoryRegionProperties>(m, "MemoryRegionProperties")
        .def(py::init<>())
        .def("TrueEnd", &rmf::types::MemoryRegionProperties::TrueEnd)
        .def("RelativeEnd", &rmf::types::MemoryRegionProperties::relativeEnd)
        .def("ToString", &rmf::types::MemoryRegionProperties::toString)
        .def_readwrite("perms", &rmf::types::MemoryRegionProperties::perms)
        .def_readwrite("parentRegionAddress", &rmf::types::MemoryRegionProperties::parentRegionAddress)
        .def_readwrite("parentRegionSize", &rmf::types::MemoryRegionProperties::parentRegionSize)
        .def_readwrite("relativeRegionAddress", &rmf::types::MemoryRegionProperties::relativeRegionAddress)
        .def_readwrite("relativeRegionSize", &rmf::types::MemoryRegionProperties::relativeRegionSize)
        .def_readwrite("pid", &rmf::types::MemoryRegionProperties::pid)
        ;

    auto mrpVecBinder = py::bind_vector<std::vector<rmf::types::MemoryRegionProperties>>(m, "mrpVecBinder");

    py::class_<rmf::types::MemoryRegionPropertiesVec>(m, "MemoryRegionPropertiesVec", mrpVecBinder)
        .def(py::init<>())
        .def(py::init([](const std::vector<rmf::types::MemoryRegionProperties>& v) {
            rmf::types::MemoryRegionPropertiesVec mrpVec;
            mrpVec.assign(v.begin(), v.end());
            return mrpVec;
        }))
    ;

    m.def("ParseMaps", &rmf::utils::ParseMaps);
}
