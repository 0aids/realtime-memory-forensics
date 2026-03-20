#include <gtest/gtest.h>
#include <pybind11/embed.h>
#include "test_helpers.hpp"
#include "logger.hpp"
#include "python_embed.hpp"
#include "types.hpp"
#include "utils.hpp"
#include <sstream>
#include <iostream>
#include <format>
#include <thread>
#include <chrono>

namespace py = pybind11;

TEST(PythonAnalyzerTest, SingleArgExecution)
{
    rmf::py::embedPythonScopedGuard guard{};

    guard.execString(R"(
analyzer = rmf.Analyzer(2)
results = analyzer.execute(lambda x: x * 2, rmf.Const(5))
result_value = results[0] if results else None
)");

    auto result = guard.getGlobals()["result_value"];
    ASSERT_FALSE(result.is_none());
    EXPECT_EQ(result.cast<int>(), 10);
}

TEST(PythonAnalyzerTest, ContainerExecution)
{
    rmf::py::embedPythonScopedGuard guard{};

    guard.execString(R"(
analyzer = rmf.Analyzer(4)
input_list = [1, 2, 3, 4, 5]
results = analyzer.execute(lambda x: x * 2, rmf.Iter(input_list))
)");

    auto results = guard.getGlobals()["results"];
    auto py_list = results.cast<py::list>();
    ASSERT_EQ(py::len(py_list), 5);
    EXPECT_EQ(py_list[0].cast<int>(), 2);
    EXPECT_EQ(py_list[1].cast<int>(), 4);
    EXPECT_EQ(py_list[2].cast<int>(), 6);
    EXPECT_EQ(py_list[3].cast<int>(), 8);
    EXPECT_EQ(py_list[4].cast<int>(), 10);
}

TEST(PythonAnalyzerTest, ReturnValuesCorrect)
{
    rmf::py::embedPythonScopedGuard guard{};

    guard.execString(R"(
analyzer = rmf.Analyzer(4)
input_list = [10, 20, 30]
results = analyzer.execute(lambda x: x + x, rmf.Iter(input_list))
)");

    auto results = guard.getGlobals()["results"];
    auto py_list = results.cast<py::list>();
    ASSERT_EQ(py::len(py_list), 3);
    EXPECT_EQ(py_list[0].cast<int>(), 20);
    EXPECT_EQ(py_list[1].cast<int>(), 40);
    EXPECT_EQ(py_list[2].cast<int>(), 60);
}

TEST(PythonAnalyzerTest, MultipleExecuteCalls)
{
    rmf::py::embedPythonScopedGuard guard{};

    guard.execString(R"(
analyzer = rmf.Analyzer(2)
results1 = analyzer.execute(lambda x: x * 2, rmf.Const(5))
results2 = analyzer.execute(lambda x: x + 1, rmf.Const(10))
result1_value = results1[0] if results1 else None
result2_value = results2[0] if results2 else None
)");

    auto result1 = guard.getGlobals()["result1_value"].cast<int>();
    auto result2 = guard.getGlobals()["result2_value"].cast<int>();
    EXPECT_EQ(result1, 10);
    EXPECT_EQ(result2, 11);
}

TEST(PythonAnalyzerTest, ParallelExecution)
{
    rmf::py::embedPythonScopedGuard guard{};

    guard.execString(R"(
import time

def cpu_bound_work(x):
    # Pure CPU work without global state
    result = 0
    for i in range(10000):
        result += i * x
    return result

analyzer = rmf.Analyzer(4)
input_list = [1] * 10
results = analyzer.execute(cpu_bound_work, rmf.Iter(input_list))

# All results should be computed (order may vary but all should complete)
had_results = len(results) == 10
)");

    auto had_results = guard.getGlobals()["had_results"].cast<bool>();
    EXPECT_TRUE(had_results);
}

TEST(PythonAnalyzerTest, MixedConstAndIter)
{
    rmf::py::embedPythonScopedGuard guard{};

    guard.execString(R"(
analyzer = rmf.Analyzer(2)

def add_multiple(a, b):
    return a + b

input_list = [1, 2, 3, 4, 5]
# Const value 10 should be used for each iteration
results = analyzer.execute(add_multiple, rmf.Iter(input_list), rmf.Const(10))
)");

    auto results = guard.getGlobals()["results"];
    auto py_list = results.cast<py::list>();
    ASSERT_EQ(py::len(py_list), 5);
    EXPECT_EQ(py_list[0].cast<int>(), 11);
    EXPECT_EQ(py_list[1].cast<int>(), 12);
    EXPECT_EQ(py_list[2].cast<int>(), 13);
    EXPECT_EQ(py_list[3].cast<int>(), 14);
    EXPECT_EQ(py_list[4].cast<int>(), 15);
}

TEST(PythonAnalyzerTest, DifferentReturnTypes)
{
    rmf::py::embedPythonScopedGuard guard{};

    guard.execString(R"(
analyzer = rmf.Analyzer(2)

int_results = analyzer.execute(lambda x: x * 2, rmf.Const(5))
double_results = analyzer.execute(lambda x: x * 1.5, rmf.Const(4.0))

int_result = int_results[0] if int_results else None
double_result = double_results[0] if double_results else None
)");

    auto int_result = guard.getGlobals()["int_result"].cast<int>();
    auto double_result =
        guard.getGlobals()["double_result"].cast<double>();
    EXPECT_EQ(int_result, 10);
    EXPECT_DOUBLE_EQ(double_result, 6.0);
}

TEST(PythonAnalyzerTest, FindString)
{
    using namespace rmf::test;

    rmf::types::MemoryRegionPropertiesVec results{};
    rmf::test::testProcess                tp;
    tp.build<staticStringTestComponent>();
    pid_t pid = tp.run();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    {
        rmf::py::embedPythonScopedGuard guard{};

        guard.execString(std::format(R"(
pid = {}
maps = rmf.getMapsFromPid(pid).filterActiveRegions(pid).filterHasPerms('r')

analyzer = rmf.Analyzer(4)

# Take snapshots using analyzer
snapshots = analyzer.execute(rmf.MakeSnapshot, rmf.Iter(maps), rmf.Const(pid))

# Filter valid snapshots and find string - sequential since nested execute is problematic
results = rmf.MemoryRegionPropertiesVec()
for snap in snapshots:
    if snap.isValid():
        found = rmf.findString(snap, "I am a small string")
        if found:
            results.extend(found)

num_results = len(results)
)",
                                     pid));

        int num_results =
            guard.getGlobals()["num_results"].cast<int>();
        EXPECT_GE(num_results, 1);
    }

    tp.stop();
}

TEST(PythonAnalyzerTest, FindNumericExact)
{
    using namespace rmf::test;

    constexpr int32_t                     targetValue = 0x99998888;

    rmf::types::MemoryRegionPropertiesVec results{};
    rmf::test::testProcess                tp;
    tp.build<staticValueComponent>(targetValue);
    pid_t pid = tp.run();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    {
        rmf::py::embedPythonScopedGuard guard{};

        guard.execString(std::format(R"(
pid = {}
maps = rmf.getMapsFromPid(pid).filterActiveRegions(pid).filterHasPerms('r')

analyzer = rmf.Analyzer(4)
snapshots = analyzer.execute(rmf.MakeSnapshot, rmf.Iter(maps), rmf.Const(pid))

# Find numeric values sequentially
results = rmf.MemoryRegionPropertiesVec()
for snap in snapshots:
    if snap.isValid():
        found = rmf.findNumericExact_i32(snap, {})
        if found:
            results.extend(found)

num_results = len(results)
)",
                                     pid, targetValue));

        int num_results =
            guard.getGlobals()["num_results"].cast<int>();
        EXPECT_GE(num_results, 1);
    }

    tp.stop();
}

TEST(PythonAnalyzerTest, FindNumericChanged)
{
    using namespace rmf::test;

    rmf::types::MemoryRegionPropertiesVec results{};
    rmf::test::testProcess                tp;
    tp.build<incrementingIntComponent>(0, 1);
    pid_t pid = tp.run();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    {
        rmf::py::embedPythonScopedGuard guard{};

        guard.execString(std::format(R"(
pid = {}
maps = rmf.getMapsFromPid(pid).filterActiveRegions(pid).filterHasPerms('rw').filterMinSize(1)

analyzer = rmf.Analyzer(4)

# First snapshot
snapshots1 = analyzer.execute(rmf.MakeSnapshot, rmf.Iter(maps), rmf.Const(pid))

import time
time.sleep(1)

# Second snapshot
snapshots2 = analyzer.execute(rmf.MakeSnapshot, rmf.Iter(maps), rmf.Const(pid))

# Find changed regions sequentially
results = rmf.MemoryRegionPropertiesVec()
for snap1, snap2 in zip(snapshots1, snapshots2):
    if snap1.isValid() and snap2.isValid():
        found = rmf.findNumericChanged_i32(snap1, snap2, 0)
        if found:
            results.extend(found)

num_results = len(results)
)",
                                     pid));

        int num_results =
            guard.getGlobals()["num_results"].cast<int>();
        EXPECT_GE(num_results, 1);
    }

    tp.stop();
}

TEST(PythonAnalyzerTest, MakeSnapshots)
{
    using namespace rmf::test;

    rmf::test::testProcess tp;
    tp.build<staticValueComponent>();
    pid_t pid = tp.run();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    {
        rmf::py::embedPythonScopedGuard guard{};

        guard.execString(std::format(R"(
pid = {}
maps = rmf.getMapsFromPid(pid).filterActiveRegions(pid).filterHasPerms('r')

analyzer = rmf.Analyzer(4)
snapshots = analyzer.execute(rmf.MakeSnapshot, rmf.Iter(maps), rmf.Const(pid))

num_snapshots = len(snapshots)
valid_snapshots = sum(1 for s in snapshots if s.isValid())
)",
                                     pid));

        int num_snapshots =
            guard.getGlobals()["num_snapshots"].cast<int>();
        int valid_snapshots =
            guard.getGlobals()["valid_snapshots"].cast<int>();

        EXPECT_GT(num_snapshots, 0);
        EXPECT_GT(valid_snapshots, 0);
    }

    tp.stop();
}

// TEST(PythonAnalyzerTest, DISABLED_PerformanceSingleVsPooledSnapshot)
// {
//     using namespace rmf::test;

//     rmf::test::testProcess tp;
//     tp.build<staticLargeEmptyComponent>();
//     pid_t pid = tp.run();

//     std::this_thread::sleep_for(std::chrono::milliseconds(100));

//     {
//         rmf::py::embedPythonScopedGuard guard{};

//         guard.execString(std::format(R"(
// import time
// pid = {}

// # Get memory maps - filter for larger regions directly
// maps = rmf.getMapsFromPid(pid).filterActiveRegions(pid).filterHasPerms('r').filterMinSize(100000)

// # Warmup
// for mrp in maps[:3]:
//     try:
//         rmf.MemorySnapshot(mrp, pid)
//     except:
//         pass

// # Single-threaded version
// start = time.perf_counter()
// single_results = []
// for mrp in maps:
//     try:
//         snap = rmf.MemorySnapshot(mrp, pid)
//         if snap.isValid():
//             single_results.append(snap)
//     except:
//         pass
// single_time = time.perf_counter() - start

// # Pooled version with 4 threads
// analyzer = rmf.Analyzer(4)
// start = time.perf_counter()
// pooled_results = analyzer.execute(rmf.MakeSnapshot, rmf.Iter(maps), rmf.Const(pid))
// pooled_time = time.perf_counter() - start

// single_time_ms = single_time * 1000
// pooled_time_ms = pooled_time * 1000
// speedup = single_time_ms / pooled_time_ms if pooled_time_ms > 0 else 0

// print(f"Single: {{single_time_ms:.2f}}ms, Pooled: {{pooled_time_ms:.2f}}ms, Speedup: {{speedup:.2f}}x")
// )",
//                                      pid));

//         double single_time_ms =
//             guard.getGlobals()["single_time_ms"].cast<double>();
//         double pooled_time_ms =
//             guard.getGlobals()["pooled_time_ms"].cast<double>();
//         double speedup = guard.getGlobals()["speedup"].cast<double>();

//         rmf_Log(rmf_Info,
//                 "Snapshot performance: Single="
//                     << single_time_ms
//                     << "ms, Pooled=" << pooled_time_ms
//                     << "ms, Speedup=" << speedup << "x");

//         EXPECT_GT(speedup, 1.3);
//     }

//     tp.stop();
// }

// TEST(PythonAnalyzerTest, DISABLED_PerformanceStringScanSpeedup)
// {
//     using namespace rmf::test;

//     rmf::test::testProcess tp;
//     tp.build<staticStringTestComponent>();
//     pid_t pid = tp.run();

//     std::this_thread::sleep_for(std::chrono::milliseconds(100));

//     {
//         rmf::py::embedPythonScopedGuard guard{};

//         guard.execString(std::format(R"(
// import time
// pid = {}

// # Get memory maps
// maps = rmf.getMapsFromPid(pid).filterActiveRegions(pid).filterHasPerms('r')

// # Warmup
// for mrp in maps[:3]:
//     try:
//         snap = rmf.MemorySnapshot(mrp, pid)
//     except:
//         pass

// # Single-threaded version
// start = time.perf_counter()
// single_results = []
// for mrp in maps:
//     try:
//         snap = rmf.MemorySnapshot(mrp, pid)
//         if snap.isValid():
//             found = rmf.findString(snap, "I am a small string")
//             if found:
//                 single_results.extend(found)
//     except:
//         pass
// single_time = time.perf_counter() - start

// # Create snapshots first (common cost)
// snapshots = []
// for mrp in maps:
//     try:
//         snap = rmf.MemorySnapshot(mrp, pid)
//         if snap.isValid():
//             snapshots.append(snap)
//     except:
//         pass

// # Pooled version
// analyzer = rmf.Analyzer(4)
// start = time.perf_counter()

// def find_in_snapshot(snap):
//     return rmf.findString(snap, "I am a small string")

// results_vecs = analyzer.execute(find_in_snapshot, rmf.Iter(snapshots))

// pooled_results = []
// for vec in results_vecs:
//     pooled_results.extend(vec)

// pooled_time = time.perf_counter() - start

// single_time_ms = single_time * 1000
// pooled_time_ms = pooled_time * 1000
// speedup = single_time_ms / pooled_time_ms if pooled_time_ms > 0 else 0

// print(f"Single: {{single_time_ms:.2f}}ms, Pooled: {{pooled_time_ms:.2f}}ms, Speedup: {{speedup:.2f}}x")
// )",
//                                      pid));

//         double single_time_ms =
//             guard.getGlobals()["single_time_ms"].cast<double>();
//         double pooled_time_ms =
//             guard.getGlobals()["pooled_time_ms"].cast<double>();
//         double speedup = guard.getGlobals()["speedup"].cast<double>();

//         rmf_Log(rmf_Info,
//                 "String scan performance: Single="
//                     << single_time_ms
//                     << "ms, Pooled=" << pooled_time_ms
//                     << "ms, Speedup=" << speedup << "x");

//         EXPECT_GT(speedup, 1.3);
//     }

//     tp.stop();
// }

// TEST(PythonAnalyzerTest, DISABLED_PerformanceNumericScanSpeedup)
// {
//     using namespace rmf::test;

//     constexpr int32_t      targetValue = 0x99998888;

//     rmf::test::testProcess tp;
//     tp.build<staticValueComponent>(targetValue);
//     pid_t pid = tp.run();

//     std::this_thread::sleep_for(std::chrono::milliseconds(100));

//     {
//         rmf::py::embedPythonScopedGuard guard{};

//         guard.execString(std::format(R"(
// import time
// pid = {}

// # Get memory maps
// maps = rmf.getMapsFromPid(pid).filterActiveRegions(pid).filterHasPerms('r')

// # Warmup
// for mrp in maps[:3]:
//     try:
//         snap = rmf.MemorySnapshot(mrp, pid)
//     except:
//         pass

// # Single-threaded version
// start = time.perf_counter()
// single_results = []
// for mrp in maps:
//     try:
//         snap = rmf.MemorySnapshot(mrp, pid)
//         if snap.isValid():
//             found = rmf.findNumericExact_i32(snap, {})
//             if found:
//                 single_results.extend(found)
//     except:
//         pass
// single_time = time.perf_counter() - start

// # Create snapshots first (common cost)
// snapshots = []
// for mrp in maps:
//     try:
//         snap = rmf.MemorySnapshot(mrp, pid)
//         if snap.isValid():
//             snapshots.append(snap)
//     except:
//         pass

// # Pooled version
// analyzer = rmf.Analyzer(4)
// start = time.perf_counter()

// def find_in_snapshot(snap):
//     return rmf.findNumericExact_i32(snap, {})

// results_vecs = analyzer.execute(find_in_snapshot, rmf.Iter(snapshots))

// pooled_results = []
// for vec in results_vecs:
//     pooled_results.extend(vec)

// pooled_time = time.perf_counter() - start

// single_time_ms = single_time * 1000
// pooled_time_ms = pooled_time * 1000
// speedup = single_time_ms / pooled_time_ms if pooled_time_ms > 0 else 0

// print(f"Single: {{single_time_ms:.2f}}ms, Pooled: {{pooled_time_ms:.2f}}ms, Speedup: {{speedup:.2f}}x")
// )",
//                                      pid, targetValue, targetValue));

//         double single_time_ms =
//             guard.getGlobals()["single_time_ms"].cast<double>();
//         double pooled_time_ms =
//             guard.getGlobals()["pooled_time_ms"].cast<double>();
//         double speedup = guard.getGlobals()["speedup"].cast<double>();

//         rmf_Log(rmf_Info,
//                 "Numeric scan performance: Single="
//                     << single_time_ms
//                     << "ms, Pooled=" << pooled_time_ms
//                     << "ms, Speedup=" << speedup << "x");

//         EXPECT_GT(speedup, 1.3);
//     }

//     tp.stop();
// }

TEST(PythonAnalyzerTest, PerformanceVaryingThreadCounts)
{
    using namespace rmf::test;

    rmf::test::testProcess tp;
    tp.build<staticLargeEmptyComponent>();
    pid_t pid = tp.run();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    {
        rmf::py::embedPythonScopedGuard guard{};

        guard.execString(std::format(R"(
import time
pid = {}

# Get memory maps - filter for larger regions directly
maps = rmf.getMapsFromPid(pid).filterActiveRegions(pid).filterHasPerms('r')

# Warmup
for mrp in maps[:3]:
    try:
        rmf.MemorySnapshot(mrp, pid)
    except:
        pass

# Run with different thread counts
times = {{}}
for num_threads in [1, 2, 4]:
    analyzer = rmf.Analyzer(num_threads)
    start = time.perf_counter()
    snapshots = analyzer.execute(rmf.MakeSnapshot, rmf.Iter(maps), rmf.Const(pid))
    elapsed = (time.perf_counter() - start) * 1000
    times[num_threads] = elapsed
    print(f"Threads: {{num_threads}}, Time: {{elapsed:.2f}}ms")

speedup_2_vs_1 = times[1] / times[2] if times[2] > 0 else 0
speedup_4_vs_1 = times[1] / times[4] if times[4] > 0 else 0
)",
                                     pid));

        double speedup_2_vs_1 =
            guard.getGlobals()["speedup_2_vs_1"].cast<double>();
        double speedup_4_vs_1 =
            guard.getGlobals()["speedup_4_vs_1"].cast<double>();

        rmf_Log(rmf_Info,
                "Thread scaling: 2 vs 1: " << speedup_2_vs_1
                                           << "x, 4 vs 1: "
                                           << speedup_4_vs_1 << "x");

        EXPECT_GT(speedup_2_vs_1, 1.0);
        EXPECT_GT(speedup_4_vs_1, 1.0);
    }

    tp.stop();
}

TEST(PythonAnalyzerTest, PerformanceVaryingThreadCountsScanning)
{
    using namespace rmf::test;

    rmf::test::testProcess tp;
    tp.build<staticLargeEmptyComponent>();
    pid_t pid = tp.run();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    {
        rmf::py::embedPythonScopedGuard guard{};

        guard.execString(std::format(R"(
import time
import rmf_py as rmf
pid = {}
rmf.SetLogLevel(rmf.LogLevel.Warning)

# Get memory maps - filter for larger regions directly
maps = rmf.getMapsFromPid(pid).filterActiveRegions(pid).filterHasPerms('r')

# Warmup
for mrp in maps[:3]:
    try:
        rmf.MemorySnapshot(mrp, pid)
    except:
        pass

timings1 = []
timings4 = []
# Run with different thread counts
def processStuff(analyzer):
    global time
    global rmf
    global maps
    global pid
    start = time.perf_counter()
    snapshots = analyzer.execute(rmf.MakeSnapshot, rmf.Iter(maps), rmf.Const(pid))

    results = analyzer.execute(rmf.findNumericExact_u8, rmf.Iter(snapshots), rmf.Const(0xf0))
    print(f"Num snapshots: {{len(snapshots)}}")
    print(f"Num results: {{len(results)}}")
    return (time.perf_counter() - start) * 1000

with rmf.Analyzer(1) as analyzer:
    for _ in range(3):
        timings1.append(processStuff(analyzer))
with rmf.Analyzer(4) as analyzer:
    for _ in range(3):
        timings4.append(processStuff(analyzer))

speedup_4_vs_1 = (sum(timings1) / len(timings1)) / (sum(timings4) / len(timings4))
)",
                                     pid));

        double speedup_4_vs_1 =
            guard.getGlobals()["speedup_4_vs_1"].cast<double>();

        rmf_Log(rmf_Info,
                "Thread scaling: 4 vs 1: " << speedup_4_vs_1 << "x");

        EXPECT_GT(speedup_4_vs_1, 1.0);
    }

    tp.stop();
}
