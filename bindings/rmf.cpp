#include <rmf/maps.hpp>
#include <rmf/process.hpp>
#include <rmf/op.hpp>

#include <nanobind/stl/bind_vector.h>
#include <nanobind/stl/string.h>
#include <nanobind/ndarray.h>
#include <nanobind/nanobind.h>

namespace nb = nanobind;

NB_MODULE(rmfpy, m)
{
    nb::enum_<rmf::Perms>(m, "Perms")
        .value("None", rmf::Perms::None)
        .value("Read", rmf::Perms::Read)
        .value("Write", rmf::Perms::Write)
        .value("Execute", rmf::Perms::Execute)
        .value("Shared", rmf::Perms::Shared)
        .def("Parse", rmf::Perms_Parse<std::string>);

    nb::class_<rmf::Map>(m, "Map")
        .def_prop_ro("name", [](const rmf::Map&m){ return m.name.get(); }) // sus
        .def_prop_ro("tbegin", &rmf::Map::tbegin)
        .def_prop_ro("tend", &rmf::Map::tend)
        .def_prop_ro("rbegin", &rmf::Map::rbegin)
        .def_prop_ro("rend", &rmf::Map::rend)
        .def_prop_ro("pbegin", &rmf::Map::pbegin)
        .def_prop_ro("pend", &rmf::Map::pend)
        .def_prop_ro("valid", &rmf::Map::valid)
        .def_ro("rsize", &rmf::Map::rSize)
        .def_ro("psize", &rmf::Map::pSize)
        .def("isLTSize", &rmf::Map::isLTSize)
        .def("isGTSize", &rmf::Map::isGTSize)
        .def("isEQSize", &rmf::Map::isEQSize)
        .def("isExactName", &rmf::Map::isExactName)
        .def("isSubName", &rmf::Map::isSubName)
        .def("isExactPerms", &rmf::Map::isExactPerms)
        .def("isHavePerms", &rmf::Map::isHavePerms)
        .def("chunkify", &rmf::Map::chunkify);


    nb::bind_vector<rmf::MapsProcVec>(m, "MapsProcVec")
        .def("getActive", &rmf::MapsProcVec::getActive);

    nb::bind_vector<std::vector<rmf::Map>>(m, "MapsVec");

    nb::class_<rmf::Process>(m, "Process")
        .def(nb::init<pid_t>())
        .def_ro("pid", &rmf::Process::pid)
        .def("getMaps", &rmf::Process::getMaps)
        .def("getSnapshot", &rmf::Process::getSnapshot)
        .def("getSnapshots",
             &rmf::Process::template getSnapshots<std::vector<rmf::Map>>)
        .def("mapGetActive", &rmf::Process::mapGetActive)
        .def("getPagemapPath", &rmf::Process::getPagemapPath);

    nb::bind_vector<rmf::Snapshot>(m, "Snapshot");

    m.def("findChanged", &rmf::findChanged);
    m.def("findUnchanged", &rmf::findUnchanged);
    m.def("findI8Changed", &rmf::findNumChanged<int8_t>);
    m.def("findI16Changed", &rmf::findNumChanged<int16_t>);
    m.def("findI32Changed", &rmf::findNumChanged<int32_t>);
    m.def("findI64Changed", &rmf::findNumChanged<int64_t>);

    m.def("findU8Changed", &rmf::findNumChanged<uint8_t>);
    m.def("findU16Changed", &rmf::findNumChanged<uint16_t>);
    m.def("findU32Changed", &rmf::findNumChanged<uint32_t>);
    m.def("findU64Changed", &rmf::findNumChanged<uint64_t>);

    m.def("findF32Changed", &rmf::findNumChanged<float>);
    m.def("findF64Changed", &rmf::findNumChanged<double>);

    m.def("findI8Unchanged", &rmf::findNumUnchanged<int8_t>);
    m.def("findI16Unchanged", &rmf::findNumUnchanged<int16_t>);
    m.def("findI32Unchanged", &rmf::findNumUnchanged<int32_t>);
    m.def("findI64Unchanged", &rmf::findNumUnchanged<int64_t>);

    m.def("findU8Unchanged", &rmf::findNumUnchanged<uint8_t>);
    m.def("findU16Unchanged", &rmf::findNumUnchanged<uint16_t>);
    m.def("findU32Unchanged", &rmf::findNumUnchanged<uint32_t>);
    m.def("findU64Unchanged", &rmf::findNumUnchanged<uint64_t>);

    m.def("findF32Unchanged", &rmf::findNumUnchanged<float>);
    m.def("findF64Unchanged", &rmf::findNumUnchanged<double>);

    m.def("findI8Exact", &rmf::findNumExact<int8_t>);
    m.def("findI16Exact", &rmf::findNumExact<int16_t>);
    m.def("findI32Exact", &rmf::findNumExact<int32_t>);
    m.def("findI64Exact", &rmf::findNumExact<int64_t>);

    m.def("findU8Exact", &rmf::findNumExact<uint8_t>);
    m.def("findU16Exact", &rmf::findNumExact<uint16_t>);
    m.def("findU32Exact", &rmf::findNumExact<uint32_t>);
    m.def("findU64Exact", &rmf::findNumExact<uint64_t>);

    m.def("findF32Exact", &rmf::findNumExact<float>);
    m.def("findF64Exact", &rmf::findNumExact<double>);

    m.def("findI8InRange", &rmf::findNumInRange<int8_t>);
    m.def("findI16InRange", &rmf::findNumInRange<int16_t>);
    m.def("findI32InRange", &rmf::findNumInRange<int32_t>);
    m.def("findI64InRange", &rmf::findNumInRange<int64_t>);

    m.def("findU8InRange", &rmf::findNumInRange<uint8_t>);
    m.def("findU16InRange", &rmf::findNumInRange<uint16_t>);
    m.def("findU32InRange", &rmf::findNumInRange<uint32_t>);
    m.def("findU64InRange", &rmf::findNumInRange<uint64_t>);

    m.def("findF32InRange", &rmf::findNumInRange<float>);
    m.def("findF64InRange", &rmf::findNumInRange<double>);

    m.def("findString", rmf::findString);
}
