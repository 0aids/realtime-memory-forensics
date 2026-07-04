#include <rmf/maps.hpp>
#include <rmf/process.hpp>
#include <nanobind/nanobind.h>

namespace nb = nanobind;

NB_MODULE(rmfpy, m) {
    nb::class_<rmf::Process>(m, "Process")
    	.def(nb::init<pid_t>())
    	.def_ro("pid", &rmf::Process::pid)
    	.def("getMaps", &rmf::Process::getMaps)
    	;
}
