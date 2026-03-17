#include "logger.hpp"
#include "multi_threading.hpp"
#include "types.hpp"
#include "utils.hpp"
#include "rmf.hpp"
#include "operations.hpp"
#include <pybind11/attr.h>
#include <pybind11/cast.h>
#include <pybind11/gil.h>
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl_bind.h>
#include <pybind11/pytypes.h>
#include <pybind11/stl.h>
#include <thread>

namespace py = pybind11;
PYBIND11_MAKE_OPAQUE(rmf::types::MemoryRegionPropertiesVec);

PYBIND11_MODULE(rmf_core_py, m, py::mod_gil_not_used())
{
    m.doc() = "python bindings for rmf (Realtime Memory Forensics)\n"
              "for use within the rmf_gui shell or externally";
    py::class_<rmf::types::MemoryRegionProperties>(
        m, "MemoryRegionProperties")
        .def("trueAddress",
             &rmf::types::MemoryRegionProperties::TrueAddress)
        .def("trueEnd", &rmf::types::MemoryRegionProperties::TrueEnd)
        .def("relativeEnd",
             &rmf::types::MemoryRegionProperties::relativeEnd)
        .def("toString",
             &rmf::types::MemoryRegionProperties::toString)
        .def("assignNewParentRegion",
             &rmf::types::MemoryRegionProperties::
                 AssignNewParentRegion);

    py::class_<rmf::types::MemorySnapshot>(m, "MemorySnapshot")
        .def(py::init(&rmf::types::MemorySnapshot::Make))
        .def("getMrp", &rmf::types::MemorySnapshot::getMrp)
        .def("isValid", &rmf::types::MemorySnapshot::isValid)
        .def("printHex", &rmf::types::MemorySnapshot::printHex,
             py::arg("charsPerLine") = 32, py::arg("numLines") = 100);

    m.def("MakeSnapshot", &rmf::types::MemorySnapshot::Make);

    py::bind_vector<rmf::types::MemoryRegionPropertiesVec>(
        m, "MemoryRegionPropertiesVec")
        .def("filterMaxSize",
             &rmf::types::MemoryRegionPropertiesVec::FilterMaxSize)
        .def("filterMinSize",
             &rmf::types::MemoryRegionPropertiesVec::FilterMinSize)
        .def("filterName",
             &rmf::types::MemoryRegionPropertiesVec::FilterName)
        .def("filterContainsName",
             &rmf::types::MemoryRegionPropertiesVec::
                 FilterContainsName)
        .def("filterExactPerms",
             &rmf::types::MemoryRegionPropertiesVec::FilterExactPerms)
        .def("filterHasPerms",
             &rmf::types::MemoryRegionPropertiesVec::FilterHasPerms)
        .def("filterNotPerms",
             &rmf::types::MemoryRegionPropertiesVec::FilterNotPerms)
        .def("breakIntoChunks",
             &rmf::types::MemoryRegionPropertiesVec::BreakIntoChunks,
             py::arg("chunkSize"), py::arg("overlap") = 0)
        .def("filterActiveRegions",
             &rmf::types::MemoryRegionPropertiesVec::
                 FilterActiveRegions)
        .def("getRegionContainingAddress",
             &rmf::types::MemoryRegionPropertiesVec::
                 GetRegionContainingAddress)
        .def("__iter__",
             [](rmf::types::MemoryRegionPropertiesVec& self)
             {
                 return py::make_iterator(self.begin(), self.end(),
                                          py::keep_alive<0, 1>());
             })
        .def("__len__", &rmf::types::MemoryRegionPropertiesVec::size)
        .def("__getitem__",
             [](rmf::types::MemoryRegionPropertiesVec& v, size_t i)
             {
                 if (i >= v.size())
                     throw py::index_error();
                 return v[i];
             });

    m.def("getMapsFromPid", &rmf::utils::getMapsFromPid);
    m.def("findChangedRegions", &rmf::op::findChangedRegions,
          py::call_guard<py::gil_scoped_release>());
    m.def("findNumericChanged_f64",
          &rmf::op::findNumericChanged<double>,
          py::call_guard<py::gil_scoped_release>());
    m.def("findNumericChanged_f32",
          &rmf::op::findNumericChanged<float>,
          py::call_guard<py::gil_scoped_release>());
    m.def("findNumericChanged_u64",
          &rmf::op::findNumericChanged<uint64_t>,
          py::call_guard<py::gil_scoped_release>());
    m.def("findNumericChanged_u32",
          &rmf::op::findNumericChanged<uint32_t>,
          py::call_guard<py::gil_scoped_release>());
    m.def("findNumericChanged_u16",
          &rmf::op::findNumericChanged<uint16_t>,
          py::call_guard<py::gil_scoped_release>());
    m.def("findNumericChanged_u8",
          &rmf::op::findNumericChanged<uint8_t>,
          py::call_guard<py::gil_scoped_release>());
    m.def("findNumericChanged_i64",
          &rmf::op::findNumericChanged<int64_t>,
          py::call_guard<py::gil_scoped_release>());
    m.def("findNumericChanged_i32",
          &rmf::op::findNumericChanged<int32_t>,
          py::call_guard<py::gil_scoped_release>());
    m.def("findNumericChanged_i16",
          &rmf::op::findNumericChanged<int16_t>,
          py::call_guard<py::gil_scoped_release>());
    m.def("findNumericChanged_i8",
          &rmf::op::findNumericChanged<int8_t>,
          py::call_guard<py::gil_scoped_release>());
    m.def("findNumericUnchanged_f64",
          &rmf::op::findNumericUnchanged<double>,
          py::call_guard<py::gil_scoped_release>());
    m.def("findNumericUnchanged_f32",
          &rmf::op::findNumericUnchanged<float>,
          py::call_guard<py::gil_scoped_release>());
    m.def("findNumericUnchanged_u64",
          &rmf::op::findNumericUnchanged<uint64_t>,
          py::call_guard<py::gil_scoped_release>());
    m.def("findNumericUnchanged_u32",
          &rmf::op::findNumericUnchanged<uint32_t>,
          py::call_guard<py::gil_scoped_release>());
    m.def("findNumericUnchanged_u16",
          &rmf::op::findNumericUnchanged<uint16_t>,
          py::call_guard<py::gil_scoped_release>());
    m.def("findNumericUnchanged_u8",
          &rmf::op::findNumericUnchanged<uint8_t>,
          py::call_guard<py::gil_scoped_release>());
    m.def("findNumericUnchanged_i64",
          &rmf::op::findNumericUnchanged<int64_t>,
          py::call_guard<py::gil_scoped_release>());
    m.def("findNumericUnchanged_i32",
          &rmf::op::findNumericUnchanged<int32_t>,
          py::call_guard<py::gil_scoped_release>());
    m.def("findNumericUnchanged_i16",
          &rmf::op::findNumericUnchanged<int16_t>,
          py::call_guard<py::gil_scoped_release>());
    m.def("findNumericUnchanged_i8",
          &rmf::op::findNumericUnchanged<int8_t>,
          py::call_guard<py::gil_scoped_release>());

    m.def("findString", &rmf::op::findString,
          py::call_guard<py::gil_scoped_release>());
    m.def("findNumericExact_f64", &rmf::op::findNumeralExact<double>,
          py::call_guard<py::gil_scoped_release>());
    m.def("findNumericExact_f32", &rmf::op::findNumeralExact<float>,
          py::call_guard<py::gil_scoped_release>());
    m.def("findNumericExact_u64",
          &rmf::op::findNumeralExact<uint64_t>,
          py::call_guard<py::gil_scoped_release>());
    m.def("findNumericExact_u32",
          &rmf::op::findNumeralExact<uint32_t>,
          py::call_guard<py::gil_scoped_release>());
    m.def("findNumericExact_u16",
          &rmf::op::findNumeralExact<uint16_t>,
          py::call_guard<py::gil_scoped_release>());
    m.def("findNumericExact_u8", &rmf::op::findNumeralExact<uint8_t>,
          py::call_guard<py::gil_scoped_release>());
    m.def("findNumericExact_i64", &rmf::op::findNumeralExact<int64_t>,
          py::call_guard<py::gil_scoped_release>());
    m.def("findNumericExact_i32", &rmf::op::findNumeralExact<int32_t>,
          py::call_guard<py::gil_scoped_release>());
    m.def("findNumericExact_i16", &rmf::op::findNumeralExact<int16_t>,
          py::call_guard<py::gil_scoped_release>());
    m.def("findNumericExact_i8", &rmf::op::findNumeralExact<int8_t>,
          py::call_guard<py::gil_scoped_release>());
    m.def("findNumericWithinRange_f64",
          &rmf::op::findNumeralWithinRange<double>,
          py::call_guard<py::gil_scoped_release>());
    m.def("findNumericWithinRange_f32",
          &rmf::op::findNumeralWithinRange<float>,
          py::call_guard<py::gil_scoped_release>());
    m.def("findNumericWithinRange_u64",
          &rmf::op::findNumeralWithinRange<uint64_t>,
          py::call_guard<py::gil_scoped_release>());
    m.def("findNumericWithinRange_u32",
          &rmf::op::findNumeralWithinRange<uint32_t>,
          py::call_guard<py::gil_scoped_release>());
    m.def("findNumericWithinRange_u16",
          &rmf::op::findNumeralWithinRange<uint16_t>,
          py::call_guard<py::gil_scoped_release>());
    m.def("findNumericWithinRange_u8",
          &rmf::op::findNumeralWithinRange<uint8_t>,
          py::call_guard<py::gil_scoped_release>());
    m.def("findNumericWithinRange_i64",
          &rmf::op::findNumeralWithinRange<int64_t>,
          py::call_guard<py::gil_scoped_release>());
    m.def("findNumericWithinRange_i32",
          &rmf::op::findNumeralWithinRange<int32_t>,
          py::call_guard<py::gil_scoped_release>());
    m.def("findNumericWithinRange_i16",
          &rmf::op::findNumeralWithinRange<int16_t>,
          py::call_guard<py::gil_scoped_release>());
    m.def("findNumericWithinRange_i8",
          &rmf::op::findNumeralWithinRange<int8_t>,
          py::call_guard<py::gil_scoped_release>());

    m.def("findPointersToRegion", &rmf::op::findPointersToRegion,
          py::call_guard<py::gil_scoped_release>());
    m.def("findPointersToRegions", &rmf::op::findPointersToRegions,
          py::call_guard<py::gil_scoped_release>());

    // Consider providing a proper wrapper for this class
    // to be interfaced with via python
    // py::class_<rmf::TaskThreadPool_t>(m, "TaskThreadPool")
    // 	.def(py::init(&rmf::TaskThreadPool_t::makeThreadPool))
    //    	.def("submitTask", &rmf::TaskThreadPool_t::SubmitTask<void>)
    //    	.def("awaitTasks", &rmf::TaskThreadPool_t::AwaitTasks)
    // ;
}
