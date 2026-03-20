#include <gtest/gtest.h>
#include <pybind11/embed.h>
#include "test_helpers.hpp"
#include "logger.hpp"
#include "python_embed.hpp"
#include "types.hpp"
#include <sstream>
#include <iostream>
#include <format>
#include <chrono>

namespace py = pybind11;

class BatcherPerformanceTest : public ::testing::Test
{
  protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(BatcherPerformanceTest, ThreadScalingSnapshot)
{
    using namespace rmf::test;

    rmf::test::testProcess tp;
    tp.build<rmf::test::staticLargeEmptyComponent>();
    pid_t pid = tp.run();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    pid_t captured_pid = pid;
    {
        rmf::py::embedPythonScopedGuard guard{};

        auto                            pythonCode = std::format(R"(
import rmf_py as rmf
import time
pid = {}

maps = rmf.getMapsFromPid(pid).filterActiveRegions(pid).filterHasPerms('r')

times = {{}}
for num_threads in [1, 2, 4]:
    batcher = rmf.Batcher(num_threads)
    start = time.perf_counter()
    snapshots = []
    for mrp in maps:
        snap = rmf.MakeSnapshot(mrp, pid)
        if snap.isValid():
            snapshots.append(snap)
    elapsed = (time.perf_counter() - start) * 1000
    times[num_threads] = elapsed

speedup_2_vs_1 = times[1] / times[2] if times[2] > 0 else 0
speedup_4_vs_1 = times[1] / times[4] if times[4] > 0 else 0
)",
                                                                 captured_pid);

        guard.execString(pythonCode);

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

TEST_F(BatcherPerformanceTest, SingleVsMultiThreadSnapshot)
{
    using namespace rmf::test;

    rmf::test::testProcess tp;
    tp.build<rmf::test::staticLargeEmptyComponent>();
    pid_t pid = tp.run();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    pid_t captured_pid = pid;
    {
        rmf::py::embedPythonScopedGuard guard{};

        auto                            pythonCode = std::format(R"(
import time
pid = {}
maps = rmf.getMapsFromPid(pid).filterActiveRegions(pid).filterHasPerms('r')

def make_snapshots():
    snapshots = []
    for mrp in maps:
        snap = rmf.MakeSnapshot(mrp, pid)
        if snap.isValid():
            snapshots.append(snap)
    return snapshots

single_times = []
for _ in range(3):
    start = time.perf_counter()
    snapshots = make_snapshots()
    single_times.append((time.perf_counter() - start) * 1000)

avg_single = sum(single_times) / len(single_times)
)",
                                                                 captured_pid);

        guard.execString(pythonCode);

        double avg_single =
            guard.getGlobals()["avg_single"].cast<double>();

        rmf_Log(rmf_Info,
                "Snapshot timing: Single=" << avg_single << "ms");
    }

    tp.stop();
}
