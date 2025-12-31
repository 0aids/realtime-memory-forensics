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

// Written by gemini because I can't be assed to bind stuff manually
template <typename T>
void bind_numeric_variants(py::module_&       m,
                           const std::string& type_suffix)
{
    using namespace rmf::op;

    // Helper to generate names like "findNumericChanged_u8"
    auto name = [&](const char* base)
    { return std::string(base) + "_" + type_suffix; };

    // 1. findNumericChanged
    m.def(name("findNumericChanged").c_str(), &findNumericChanged<T>,
          py::call_guard<py::gil_scoped_release>(), py::arg("snap1"),
          py::arg("snap2"), py::arg("minDifference"),
          "Find regions that changed by at least minDifference");

    // 2. findNumericUnchanged
    m.def(name("findNumericUnchanged").c_str(),
          &findNumericUnchanged<T>,
          py::call_guard<py::gil_scoped_release>(), py::arg("snap1"),
          py::arg("snap2"), py::arg("maxDifference"),
          "Find regions that changed by at most maxDifference");

    // 3. findNumeralExact
    m.def(name("findNumeralExact").c_str(), &findNumeralExact<T>,
          py::call_guard<py::gil_scoped_release>(), py::arg("snap1"),
          py::arg("number"),
          "Find regions containing exactly this number");

    // 4. findNumeralWithinRange
    m.def(name("findNumeralWithinRange").c_str(),
          &findNumeralWithinRange<T>,
          py::call_guard<py::gil_scoped_release>(), py::arg("snap1"),
          py::arg("min"), py::arg("max"),
          "Find regions containing numbers within [min, max]");
}

// Helper to check if an object is a list/tuple (but not a string)
bool is_sequence(py::handle h)
{
    return py::isinstance<py::sequence>(h) &&
        !py::isinstance<py::str>(h) && !py::isinstance<py::bytes>(h);
}

void bind_analyzer(py::module_& m)
{
    using namespace rmf;
    py::class_<Analyzer>(m, "Analyzer")
        .def(py::init<size_t>())
        .def(
            "execute",
            [](Analyzer& self, py::function func, py::args args)
            {
                // 1. Determine Batch Size (Vectorization)
                size_t batch_size = 1;
                for (const auto& arg : args)
                {
                    if (is_sequence(arg))
                    {
                        size_t len = py::len(arg);
                        if (batch_size != 1 && batch_size != len)
                        {
                            throw std::runtime_error(
                                "Argument lists must have matching "
                                "lengths");
                        }
                        batch_size = len;
                    }
                }

                // 2. Prepare Futures container
                // We use std::future<py::object> because the return type is dynamic
                std::vector<std::future<py::object>> futures;
                futures.reserve(batch_size);

                // 3. Scatter: Create and Submit Tasks
                for (size_t i = 0; i < batch_size; ++i)
                {

                    // Construct arguments for this specific iteration
                    py::list call_args;
                    for (const auto& arg : args)
                    {
                        if (is_sequence(arg))
                        {
                            // Extract element i from list
                            call_args.append(
                                arg.cast<py::sequence>()[i]);
                        }
                        else
                        {
                            // Use scalar as is
                            call_args.append(arg);
                        }
                    }

                    // Create a lambda to run on the thread pool
                    // CRITICAL: We capture func and args by value
                    auto task_wrapper = [func,
                                         call_args]() -> py::object
                    {
                        // WE ARE NOW IN A WORKER THREAD.
                        // We must acquire the GIL to inspect py::objects and call python functions.
                        py::gil_scoped_acquire acquire;

                        // Call the function (Use *call_args to unpack the list into arguments)
                        return func(*call_args);
                    };

                    // Submit to your underlying implementation
                    // Assuming your m_impl->tp.SubmitTask can accept std::function or similar.
                    // You might need to expose a helper method on Analyzer to accept std::function<py::object()>
                    // For this example, I assume a standard future-based submission:
                    auto task = Task_t<py::object>(task_wrapper);
                    self.getImpl()->tp.SubmitTask(task, [](){
                        rmf_Log(rmf_Warning, "Task Enqueue failed! Yielding for 1s");
                        using namespace std::chrono_literals;
                        py::gil_scoped_release release;
                        std::this_thread::yield(); 
                        std::this_thread::sleep_for(1s);
                    });
                    futures.emplace_back(std::move(task.getFuture()));
                    // --- FIX: PERIODIC GIL RELEASE ---
                    // Every few submissions, release the GIL to let workers start.
                    // This prevents the queue from filling up and blocking the main thread,
                    // and allows parallel processing to start immediately.
                    rmf_Log(rmf_Debug, "python bindings: temporarily release gil to start other threads")
                    py::gil_scoped_release release;
                    // Yield to ensure OS scheduler gives workers a chance to grab the GIL
                    std::this_thread::yield(); 
                }

                // 4. Gather: Wait for results
                py::list results;
                for (auto& f : futures)
                {
                    // Release GIL while waiting for C++ threads to finish!
                    // Otherwise, the main thread holds GIL -> Worker waits for GIL -> Deadlock/Serial execution.
                    py::object result;
                    {
                        py::gil_scoped_release release;
                        result = f.get();
                    }
                    results.append(result);
                }

                return results;
            });
}

// Ai ends here

PYBIND11_MAKE_OPAQUE(std::vector<rmf::types::MemoryRegionProperties>);

PYBIND11_MODULE(rmf_py, m, py::mod_gil_not_used())
{
    m.doc() = "Realtime Memory Forensics - Python Bindings";
    py::enum_<rmf::types::Perms>(m, "Perms", py::module_local())
        .value("none", rmf::types::Perms::None)
        .value("read", rmf::types::Perms::Read)
        .value("write", rmf::types::Perms::Write)
        .value("execute", rmf::types::Perms::Execute)
        .value("shared", rmf::types::Perms::Shared);

    py::enum_<rmf_LogLevel>(m, "LogLevel", py::module_local())
        .value("Error", rmf_Error)
        .value("Warning", rmf_Warning)
        .value("Info", rmf_Info)
        .value("Verbose", rmf_Verbose)
        .value("Debug", rmf_Debug)
        .value("Reset", rmf_Reset)
        ;
    m.def("SetLogLevel", [](rmf_LogLevel level) {
        rmf::g_logLevel = level;
    }, "Updates the global C++ log level");

    py::class_<rmf::types::MemoryRegionProperties>(
        m, "MemoryRegionProperties")
        .def(py::init<>())
        .def("BreakIntoChunks", &rmf::types::MemoryRegionProperties::BreakIntoChunks)
        .def("TrueAddress",
             &rmf::types::MemoryRegionProperties::TrueAddress)
        .def("TrueEnd", &rmf::types::MemoryRegionProperties::TrueEnd)
        .def("RelativeEnd",
             &rmf::types::MemoryRegionProperties::relativeEnd)
        .def("ToString",
             &rmf::types::MemoryRegionProperties::toString)
        .def_readwrite("perms",
                       &rmf::types::MemoryRegionProperties::perms)
        .def_readwrite(
            "parentRegionAddress",
            &rmf::types::MemoryRegionProperties::parentRegionAddress)
        .def_readwrite(
            "parentRegionSize",
            &rmf::types::MemoryRegionProperties::parentRegionSize)
        .def_readwrite("relativeRegionAddress",
                       &rmf::types::MemoryRegionProperties::
                           relativeRegionAddress)
        .def_readwrite(
            "relativeRegionSize",
            &rmf::types::MemoryRegionProperties::relativeRegionSize)
        .def_readwrite("pid",
                       &rmf::types::MemoryRegionProperties::pid);

    auto mrpVecBinder = py::bind_vector<
        std::vector<rmf::types::MemoryRegionProperties>>(
        m, "mrpVecBinder");

    py::class_<rmf::types::MemoryRegionPropertiesVec>(
        m, "MemoryRegionPropertiesVec", mrpVecBinder)
        .def(py::init<>())
        .def(py::init(
            [](const std::vector<rmf::types::MemoryRegionProperties>&
                   v)
            {
                rmf::types::MemoryRegionPropertiesVec mrpVec;
                mrpVec.assign(v.begin(), v.end());
                return mrpVec;
            }))
        .def("BreakIntoChunks", &rmf::types::MemoryRegionPropertiesVec::BreakIntoChunks)
        .def("FilterMaxSize",
             &rmf::types::MemoryRegionPropertiesVec::FilterMaxSize)
        .def("FilterMinSize",
             &rmf::types::MemoryRegionPropertiesVec::FilterMinSize)
        .def("FilterName",
             &rmf::types::MemoryRegionPropertiesVec::FilterName)
        .def("FilterContainsName",
             &rmf::types::MemoryRegionPropertiesVec::
                 FilterContainsName)
        .def("FilterPerms",
             &rmf::types::MemoryRegionPropertiesVec::FilterPerms)
        .def("FilterHasPerms",
             &rmf::types::MemoryRegionPropertiesVec::FilterHasPerms)
        .def("FilterNotPerms",
             &rmf::types::MemoryRegionPropertiesVec::FilterNotPerms);

    m.def("ParseMaps", &rmf::utils::ParseMaps);
    m.def("CompressNestedMrpVec", 
        [](const py::list& listOfVecs) {
            std::vector<rmf::types::MemoryRegionPropertiesVec> cpp_vec;
            cpp_vec.reserve(listOfVecs.size());

            for (auto handle : listOfVecs) {
                cpp_vec.push_back(handle.cast<rmf::types::MemoryRegionPropertiesVec>());
            }

            return rmf::utils::CompressNestedMrpVec(cpp_vec);
        },
        py::arg("mrp_vecs"),
        "Compresses a list of MemoryRegionPropertiesVec objects."
    );

    py::class_<rmf::types::MemorySnapshot>(m, "MemorySnapshot")
        .def(py::init(&rmf::types::MemorySnapshot::Make),
             py::call_guard<py::gil_scoped_release>())
        .def("getMrp", &rmf::types::MemorySnapshot::getMrp)
        .def("printHex", &rmf::types::MemorySnapshot::printHex)
        .def("isValid", &rmf::types::MemorySnapshot::isValid)
        .def_property_readonly(
            "data",
            [](const rmf::types::MemorySnapshot& self)
            {
                auto impl = self.getImpl();
                // Uses pycast to make a copy of the shared ptr, and assign that to the lifetime of the array that's returned.
                return py::array(impl->mc_data.size(),
                                 impl->mc_data.data(),
                                 py::cast(impl));
            });

    // Bind operations
    m.def("findChangedRegions", &rmf::op::findChangedRegions, py::call_guard<py::gil_scoped_release>());
    m.def("findUnchangedRegions", &rmf::op::findUnchangedRegions, py::call_guard<py::gil_scoped_release>());
    m.def("findString", &rmf::op::findString, py::call_guard<py::gil_scoped_release>());
    m.def("findPointersToRegion", &rmf::op::findPointersToRegion, py::call_guard<py::gil_scoped_release>());
    m.def("findPointersToRegions", &rmf::op::findPointersToRegions, py::call_guard<py::gil_scoped_release>());

    // Bind some utilities

    // Written by AI
    // Binding templated operations.
    // Bind Floating Point Types
    bind_numeric_variants<float>(m, "f32");
    bind_numeric_variants<double>(m, "f64");

    // Bind Unsigned Integer Types
    bind_numeric_variants<uint8_t>(m, "u8");
    bind_numeric_variants<uint16_t>(m, "u16");
    bind_numeric_variants<uint32_t>(m, "u32");
    bind_numeric_variants<uint64_t>(m, "u64");

    // Bind Signed Integer Types
    bind_numeric_variants<int8_t>(m, "i8");
    bind_numeric_variants<int16_t>(m, "i16");
    bind_numeric_variants<int32_t>(m, "i32");
    bind_numeric_variants<int64_t>(m, "i64");

    // Bind the analyzer
    bind_analyzer(m);
}
