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

namespace py = pybind11;

TEST(PythonBindingsTest, getMapsFromPid)
{
    using namespace rmf::test;

    rmf::types::MemoryRegionPropertiesVec results{};
    rmf::test::testProcess                tp;
    tp.build<rmf::test::staticValueComponent>();
    pid_t       pid = tp.run();
    std::string stdoutBuffer;
    std::string stderrBuffer;

    {
        rmf::py::embedPythonScopedGuard guard{};

        if (guard.execString(
                std::format("maps = rmf.getMapsFromPid({})", pid)))
        {
            results =
                guard.getLocals()["maps"]
                    .cast<rmf::types::MemoryRegionPropertiesVec>();
        }
    }

    tp.stop();

    rmf_Log(rmf_Info, "Number of results found: " << results.size());
    EXPECT_GT(results.size(), 0);
}

TEST(PythonBindingsTest, filterHasPerms)
{
    using namespace rmf::test;

    rmf::types::MemoryRegionPropertiesVec results{};
    rmf::test::testProcess                tp;
    tp.build<rmf::test::staticValueComponent>();
    pid_t pid = tp.run();

    {
        rmf::py::embedPythonScopedGuard guard{};

        guard.execString(
            std::format("maps = rmf.getMapsFromPid({})", pid));
        guard.execString(R"(readable = maps.filterHasPerms("r"))");

        results = guard.getLocals()["readable"]
                      .cast<rmf::types::MemoryRegionPropertiesVec>();
    }

    tp.stop();

    EXPECT_GT(results.size(), 0);
}

TEST(PythonBindingsTest, breakIntoChunks)
{
    using namespace rmf::test;

    rmf::types::MemoryRegionPropertiesVec results{};
    rmf::test::testProcess                tp;
    tp.build<rmf::test::staticValueComponent>();
    pid_t pid = tp.run();

    {
        rmf::py::embedPythonScopedGuard guard{};

        guard.execString(
            std::format("maps = rmf.getMapsFromPid({})", pid));
        guard.execString(R"(readable = maps.filterHasPerms("r"))");
        guard.execString(
            R"(chunked = readable.breakIntoChunks(0x1000))");

        results = guard.getLocals()["chunked"]
                      .cast<rmf::types::MemoryRegionPropertiesVec>();
    }

    tp.stop();

    EXPECT_GT(results.size(), 0);
}

TEST(PythonBindingsTest, filterActiveRegions)
{
    using namespace rmf::test;

    rmf::types::MemoryRegionPropertiesVec results{};
    rmf::test::testProcess                tp;
    tp.build<rmf::test::staticValueComponent>();
    pid_t pid = tp.run();

    {
        rmf::py::embedPythonScopedGuard guard{};

        guard.execString(
            std::format("maps = rmf.getMapsFromPid({})", pid));
        guard.execString(std::format(
            "active = maps.filterActiveRegions({})", pid));

        results = guard.getLocals()["active"]
                      .cast<rmf::types::MemoryRegionPropertiesVec>();
    }

    tp.stop();

    EXPECT_GT(results.size(), 0);
}

TEST(PythonBindingsTest, findNumericExact_i32)
{
    using namespace rmf::test;

    constexpr int32_t                     targetValue = 0x99998888;

    rmf::types::MemoryRegionPropertiesVec results{};
    rmf::test::testProcess                tp;
    tp.build<rmf::test::staticValueComponent>(targetValue);
    pid_t pid = tp.run();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    {
        rmf::py::embedPythonScopedGuard guard(
            rmf::py::RedirectPolicy::None);

        guard.execString(std::format(
            R"(
maps = rmf.getMapsFromPid({})
readable = maps.filterHasPerms("r")
snapshots = []
for mrp in readable:
    try:
        snap = rmf.MemorySnapshot(mrp, {})
        if snap.isValid():
            snapshots.append(snap)
    except:
        pass

results = rmf.MemoryRegionPropertiesVec()

for snap in snapshots:
    print(f"snapshot mrp: {{snap.getMrp().toString()}}")
    found = rmf.findNumericExact_i32(snap, {})
    if found:
        print(f"Found: {{type(found)}}")
        print(f"Found {{len(found)}} elements")
        results.extend(found)
)",
            pid, pid, targetValue));

        if (py::len(guard.getLocals()["results"]) != 0)
            results =
                guard.getLocals()["results"]
                    .cast<rmf::types::MemoryRegionPropertiesVec>();
        else
            rmf_Log(rmf_Error, "Found no results!");
    }

    tp.stop();

    EXPECT_GE(results.size(), 2);
}

TEST(PythonBindingsTest, findNumericExact_i64)
{
    using namespace rmf::test;

    constexpr int64_t targetValue = 0xfedcba9876543210;

    rmf::types::MemoryRegionPropertiesVec results{};
    rmf::test::testProcess                tp;
    tp.build<rmf::test::staticValueComponent>(0, targetValue);
    pid_t pid = tp.run();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    {
        rmf::py::embedPythonScopedGuard guard{};

        guard.execString(
            std::format("maps = rmf.getMapsFromPid({})", pid));
        guard.execString(R"(readable = maps.filterHasPerms("r"))");

        guard.execString(std::format(R"(
snapshots = []
for mrp in readable:
    try:
        snap = rmf.MemorySnapshot(mrp, {})
        if snap.isValid():
            snapshots.append(snap)
    except:
        pass
)",
                                     pid));

        guard.execString(
            R"(results = rmf.MemoryRegionPropertiesVec())");

        guard.execString(std::format(R"(
for snap in snapshots:
    found = rmf.findNumericExact_i64(snap, {})
    if found:
        results.extend(found)
)",
                                     targetValue));

        results = guard.getLocals()["results"]
                      .cast<rmf::types::MemoryRegionPropertiesVec>();
    }

    tp.stop();

    EXPECT_GE(results.size(), 2);
}

TEST(PythonBindingsTest, findNumericExact_f32)
{
    using namespace rmf::test;

    constexpr float                       targetValue = 3.14159f;

    rmf::types::MemoryRegionPropertiesVec results{};
    rmf::test::testProcess                tp;
    tp.build<rmf::test::staticValueComponent>(0, 0, targetValue, 0.0);
    pid_t pid = tp.run();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    {
        rmf::py::embedPythonScopedGuard guard{};

        guard.execString(
            std::format("maps = rmf.getMapsFromPid({})", pid));
        guard.execString(R"(readable = maps.filterHasPerms("r"))");

        guard.execString(std::format(R"(
snapshots = []
for mrp in readable:
    try:
        snap = rmf.MemorySnapshot(mrp, {})
        if snap.isValid():
            snapshots.append(snap)
    except:
        pass
)",
                                     pid));

        guard.execString(
            R"(results = rmf.MemoryRegionPropertiesVec())");

        guard.execString(std::format(R"(
for snap in snapshots:
    found = rmf.findNumericExact_f32(snap, {})
    if found:
        results.extend(found)
)",
                                     targetValue));

        results = guard.getLocals()["results"]
                      .cast<rmf::types::MemoryRegionPropertiesVec>();
    }

    tp.stop();

    EXPECT_GE(results.size(), 2);
}

TEST(PythonBindingsTest, findNumericExact_f64)
{
    using namespace rmf::test;

    constexpr double                      targetValue = 2.718281828;

    rmf::types::MemoryRegionPropertiesVec results{};
    rmf::test::testProcess                tp;
    tp.build<rmf::test::staticValueComponent>(0, 0, 0.0f,
                                              targetValue);
    pid_t pid = tp.run();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    {
        rmf::py::embedPythonScopedGuard guard{};

        guard.execString(
            std::format("maps = rmf.getMapsFromPid({})", pid));
        guard.execString(R"(readable = maps.filterHasPerms("r"))");

        guard.execString(std::format(R"(
snapshots = []
for mrp in readable:
    try:
        snap = rmf.MemorySnapshot(mrp, {})
        if snap.isValid():
            snapshots.append(snap)
    except:
        pass
)",
                                     pid));

        guard.execString(
            R"(results = rmf.MemoryRegionPropertiesVec())");

        guard.execString(std::format(R"(
import struct
packed = struct.pack('d', {})
val = struct.unpack('q', packed)[0]
for snap in snapshots:
    found = rmf.findNumericExact_i64(snap, val)
    if found:
        results.extend(found)
)",
                                     targetValue));

        results = guard.getLocals()["results"]
                      .cast<rmf::types::MemoryRegionPropertiesVec>();
    }

    tp.stop();

    EXPECT_GE(results.size(), 2);
}

TEST(PythonBindingsTest, findString)
{
    using namespace rmf::test;

    rmf::types::MemoryRegionPropertiesVec results{};
    rmf::test::testProcess                tp;
    tp.build<rmf::test::staticStringTestComponent>();
    pid_t pid = tp.run();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    {
        rmf::py::embedPythonScopedGuard guard{};

        guard.execString(
            std::format("maps = rmf.getMapsFromPid({})", pid));
        guard.execString(R"(readable = maps.filterHasPerms("r"))");

        guard.execString(std::format(R"(
snapshots = []
for mrp in readable:
    try:
        snap = rmf.MemorySnapshot(mrp, {})
        if snap.isValid():
            snapshots.append(snap)
    except:
        pass
)",
                                     pid));

        guard.execString(
            R"(results = rmf.MemoryRegionPropertiesVec())");

        guard.execString(R"(
for snap in snapshots:
    found = rmf.findString(snap, "I am a small string")
    if found:
        results.extend(found)
)");

        results = guard.getLocals()["results"]
                      .cast<rmf::types::MemoryRegionPropertiesVec>();
    }

    tp.stop();

    EXPECT_GE(results.size(), 2);
}

TEST(PythonBindingsTest, findChangedRegions)
{
    using namespace rmf::test;

    rmf::types::MemoryRegionPropertiesVec results{};
    rmf::test::testProcess                tp;
    tp.build<rmf::test::incrementingIntComponent>();
    pid_t pid = tp.run();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    {
        rmf::py::embedPythonScopedGuard guard{};

        guard.execString(std::format(R"(
pid = {}
maps = rmf.getMapsFromPid(pid)
filtered = maps.filterHasPerms("rw").filterActiveRegions(pid)
snapshots1 = []
for mrp in filtered:
    snapshot = rmf.MemorySnapshot(mrp, pid)
    if snapshot.isValid():
        snapshots1.append(snapshot)

from time import sleep
sleep(2)

snapshots2 = []
for mrp in filtered:
    snapshot = rmf.MemorySnapshot(mrp, pid)
    if snapshot.isValid():
        snapshots2.append(snapshot)

results = rmf.MemoryRegionPropertiesVec()
for snap1, snap2 in zip(snapshots1, snapshots2):
    results.extend(rmf.findChangedRegions(snap1, snap2, 32))
)",
                                     pid));

        results = guard.getLocals()["results"]
                      .cast<rmf::types::MemoryRegionPropertiesVec>();
    }

    tp.stop();

    EXPECT_GE(results.size(), 1);
}

TEST(PythonBindingsTest, findNumericChanged_i32)
{
    using namespace rmf::test;

    rmf::types::MemoryRegionPropertiesVec results{};
    rmf::test::testProcess                tp;
    tp.build<rmf::test::incrementingIntComponent>(0, 1);
    pid_t pid = tp.run();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    {
        rmf::py::embedPythonScopedGuard guard{};

        guard.execString(
            std::format("maps = rmf.getMapsFromPid({})", pid));
        guard.execString(R"(readable = maps.filterHasPerms("rw"))");

        guard.execString(std::format(R"(
mrp = readable[0]
snap1 = rmf.MemorySnapshot(mrp, {})
)",
                                     pid));

        std::this_thread::sleep_for(std::chrono::seconds(2));

        guard.execString(std::format(R"(
snap2 = rmf.MemorySnapshot(mrp, {})
)",
                                     pid));

        guard.execString(
            R"(results = rmf.findNumericChanged_i32(snap1, snap2, 0))");

        results = guard.getLocals()["results"]
                      .cast<rmf::types::MemoryRegionPropertiesVec>();
    }

    tp.stop();
}

TEST(PythonBindingsTest, findNumericUnchanged_i32)
{
    using namespace rmf::test;

    rmf::types::MemoryRegionPropertiesVec results{};
    rmf::test::testProcess                tp;
    tp.build<rmf::test::staticValueComponent>();
    pid_t pid = tp.run();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    {
        rmf::py::embedPythonScopedGuard guard{};

        guard.execString(
            std::format("maps = rmf.getMapsFromPid({})", pid));
        guard.execString(R"(readable = maps.filterHasPerms("r"))");

        guard.execString(std::format(R"(
mrp = readable[0]
snap1 = rmf.MemorySnapshot(mrp, {})
)",
                                     pid));

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        guard.execString(std::format(R"(
snap2 = rmf.MemorySnapshot(mrp, {})
)",
                                     pid));

        guard.execString(
            R"(results = rmf.findNumericUnchanged_i32(snap1, snap2, 256))");

        results = guard.getLocals()["results"]
                      .cast<rmf::types::MemoryRegionPropertiesVec>();
    }

    tp.stop();

    EXPECT_GE(results.size(), 1);
}

TEST(PythonBindingsTest, findNumericWithinRange_i32)
{
    using namespace rmf::test;

    constexpr int32_t                     targetValue = 0x12345678;

    rmf::types::MemoryRegionPropertiesVec results{};
    rmf::test::testProcess                tp;
    tp.build<rmf::test::staticValueComponent>(targetValue);
    pid_t pid = tp.run();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    {
        rmf::py::embedPythonScopedGuard guard{};

        guard.execString(
            std::format("maps = rmf.getMapsFromPid({})", pid));
        guard.execString(R"(readable = maps.filterHasPerms("r"))");

        guard.execString(std::format(R"(
snapshots = []
for mrp in readable:
    try:
        snap = rmf.MemorySnapshot(mrp, {})
        if snap.isValid():
            snapshots.append(snap)
    except:
        pass
)",
                                     pid));

        guard.execString(
            R"(results = rmf.MemoryRegionPropertiesVec())");

        guard.execString(R"(
for snap in snapshots:
    found = rmf.findNumericWithinRange_i32(snap, 0x12340000, 0x12350000)
    if found:
        results.extend(found)
)");

        results = guard.getLocals()["results"]
                      .cast<rmf::types::MemoryRegionPropertiesVec>();
    }

    tp.stop();

    EXPECT_GE(results.size(), 2);
}

TEST(PythonBindingsTest, getRegionContainingAddress)
{
    using namespace rmf::test;

    rmf::types::MemoryRegionPropertiesVec results{};
    rmf::test::testProcess                tp;
    tp.build<rmf::test::staticValueComponent>();
    pid_t pid = tp.run();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    {
        rmf::py::embedPythonScopedGuard guard{};

        guard.execString(
            std::format("maps = rmf.getMapsFromPid({})", pid));
        guard.execString(R"(readable = maps.filterHasPerms("r"))");

        guard.execString(R"(
addr = readable[0].trueAddress()
region = readable.getRegionContainingAddress(addr)
)");

        auto region = guard.getLocals()["region"];
        EXPECT_TRUE(region.is_none() == false);
    }

    tp.stop();
}

TEST(PythonBindingsTest, analyzerInitialTest)
{
    using namespace rmf::test;

    rmf::types::MemoryRegionPropertiesVec results{};
    rmf::test::testProcess                tp;
    tp.build<rmf::test::staticValueComponent>();
    pid_t pid = tp.run();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    {
        rmf::py::embedPythonScopedGuard guard{};

        guard.execString(std::format(R"(
pid = {}
analyzer = rmf.Analyzer()
maps = rmf.getMapsFromPid(pid).filterActiveRegions(pid).filterHasPerms('w')

snapshots = analyzer.execute(rmf.MakeSnapshot, rmf.Iter(maps), rmf.Const(pid))
numSnapshots = len(snapshots)
    )",
                                     pid));
        int numSnapshots =
            guard.getLocals()["numSnapshots"].cast<int>();
        EXPECT_GT(numSnapshots, 0);
    }

    tp.stop();
}
