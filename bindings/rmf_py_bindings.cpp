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
	py::class_<rmf::types::MemoryRegionProperties>(m, "MemoryRegionProperties")
		.def("trueAddress", &rmf::types::MemoryRegionProperties::TrueAddress)
		.def("trueEnd", &rmf::types::MemoryRegionProperties::TrueEnd)
		.def("relativeEnd", &rmf::types::MemoryRegionProperties::relativeEnd)
		.def("toString", &rmf::types::MemoryRegionProperties::toString)
		.def("assignNewParentRegion", &rmf::types::MemoryRegionProperties::AssignNewParentRegion)
		;

	py::class_<rmf::types::MemorySnapshot>(m, "MemorySnapshot")
		.def(py::init(&rmf::types::MemorySnapshot::Make))
		.def("getMrp", &rmf::types::MemorySnapshot::getMrp)
		.def("isValid", &rmf::types::MemorySnapshot::isValid)
		.def("printHex", &rmf::types::MemorySnapshot::printHex,
			py::arg("charsPerLine") = 32, py::arg("numLines") = 100)
		;

	py::bind_vector<rmf::types::MemoryRegionPropertiesVec>(m ,"MemoryRegionPropertiesVec")
		.def("filterMaxSize", &rmf::types::MemoryRegionPropertiesVec::FilterMaxSize)
		.def("filterMinSize", &rmf::types::MemoryRegionPropertiesVec::FilterMinSize)
		.def("filterName", &rmf::types::MemoryRegionPropertiesVec::FilterName)
		.def("filterContainsName", &rmf::types::MemoryRegionPropertiesVec::FilterContainsName)
		.def("filterExactPerms", &rmf::types::MemoryRegionPropertiesVec::FilterExactPerms)
		.def("filterHasPerms", &rmf::types::MemoryRegionPropertiesVec::FilterHasPerms)
		.def("filterNotPerms", &rmf::types::MemoryRegionPropertiesVec::FilterNotPerms)
		.def("breakIntoChunks", &rmf::types::MemoryRegionPropertiesVec::BreakIntoChunks,
			py::arg("chunkSize"), py::arg("overlap") = 0)
		.def("filterActiveRegions", &rmf::types::MemoryRegionPropertiesVec::FilterActiveRegions)
		.def("getRegionContainingAddress", &rmf::types::MemoryRegionPropertiesVec::GetRegionContainingAddress)
		.def("__iter__", [](rmf::types::MemoryRegionPropertiesVec& self)
		{
    		return py::make_iterator(self.begin(), self.end(), py::keep_alive<0, 1>());
		})
		.def("__len__", &rmf::types::MemoryRegionPropertiesVec::size)
		.def("__getitem__", [](rmf::types::MemoryRegionPropertiesVec &v, size_t i) {
            if (i >= v.size()) throw py::index_error();
                return v[i];
        })
		;

	m.def("getMapsFromPid", &rmf::utils::getMapsFromPid);
	m.def("findChangedRegions", &rmf::op::findChangedRegions);
	m.def("findNumericChanged_f64", &rmf::op::findNumericChanged<double>);
	m.def("findNumericChanged_f32", &rmf::op::findNumericChanged<float>);
	m.def("findNumericChanged_u64", &rmf::op::findNumericChanged<uint64_t>);
	m.def("findNumericChanged_u32", &rmf::op::findNumericChanged<uint32_t>);
	m.def("findNumericChanged_u16", &rmf::op::findNumericChanged<uint16_t>);
	m.def("findNumericChanged_u8", &rmf::op::findNumericChanged<uint8_t>);
	m.def("findNumericChanged_i64", &rmf::op::findNumericChanged<int64_t>);
	m.def("findNumericChanged_i32", &rmf::op::findNumericChanged<int32_t>);
	m.def("findNumericChanged_i16", &rmf::op::findNumericChanged<int16_t>);
	m.def("findNumericChanged_i8", &rmf::op::findNumericChanged<int8_t>);
	m.def("findNumericUnchanged_f64", &rmf::op::findNumericUnchanged<double>);
	m.def("findNumericUnchanged_f32", &rmf::op::findNumericUnchanged<float>);
	m.def("findNumericUnchanged_u64", &rmf::op::findNumericUnchanged<uint64_t>);
	m.def("findNumericUnchanged_u32", &rmf::op::findNumericUnchanged<uint32_t>);
	m.def("findNumericUnchanged_u16", &rmf::op::findNumericUnchanged<uint16_t>);
	m.def("findNumericUnchanged_u8", &rmf::op::findNumericUnchanged<uint8_t>);
	m.def("findNumericUnchanged_i64", &rmf::op::findNumericUnchanged<int64_t>);
	m.def("findNumericUnchanged_i32", &rmf::op::findNumericUnchanged<int32_t>);
	m.def("findNumericUnchanged_i16", &rmf::op::findNumericUnchanged<int16_t>);
	m.def("findNumericUnchanged_i8", &rmf::op::findNumericUnchanged<int8_t>);

	m.def("findString", &rmf::op::findString);
	m.def("findNumericExact_f64", &rmf::op::findNumeralExact<double>);
	m.def("findNumericExact_f32", &rmf::op::findNumeralExact<float>);
	m.def("findNumericExact_u64", &rmf::op::findNumeralExact<uint64_t>);
	m.def("findNumericExact_u32", &rmf::op::findNumeralExact<uint32_t>);
	m.def("findNumericExact_u16", &rmf::op::findNumeralExact<uint16_t>);
	m.def("findNumericExact_u8", &rmf::op::findNumeralExact<uint8_t>);
	m.def("findNumericExact_i64", &rmf::op::findNumeralExact<int64_t>);
	m.def("findNumericExact_i32", &rmf::op::findNumeralExact<int32_t>);
	m.def("findNumericExact_i16", &rmf::op::findNumeralExact<int16_t>);
	m.def("findNumericExact_i8", &rmf::op::findNumeralExact<int8_t>);
	m.def("findNumericWithinRange_f64", &rmf::op::findNumeralWithinRange<double>);
	m.def("findNumericWithinRange_f32", &rmf::op::findNumeralWithinRange<float>);
	m.def("findNumericWithinRange_u64", &rmf::op::findNumeralWithinRange<uint64_t>);
	m.def("findNumericWithinRange_u32", &rmf::op::findNumeralWithinRange<uint32_t>);
	m.def("findNumericWithinRange_u16", &rmf::op::findNumeralWithinRange<uint16_t>);
	m.def("findNumericWithinRange_u8", &rmf::op::findNumeralWithinRange<uint8_t>);
	m.def("findNumericWithinRange_i64", &rmf::op::findNumeralWithinRange<int64_t>);
	m.def("findNumericWithinRange_i32", &rmf::op::findNumeralWithinRange<int32_t>);
	m.def("findNumericWithinRange_i16", &rmf::op::findNumeralWithinRange<int16_t>);
	m.def("findNumericWithinRange_i8", &rmf::op::findNumeralWithinRange<int8_t>);

	m.def("findPointersToRegion", &rmf::op::findPointersToRegion);
	m.def("findPointersToRegions", &rmf::op::findPointersToRegions);
}
