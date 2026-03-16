#include <gtest/gtest.h>
#include <pybind11/embed.h>
#include "test_helpers.hpp"
#include "logger.hpp"
#include "python_embed.hpp"
#include "types.hpp"
#include <sstream>
#include <iostream>
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

        if (guard.execString("maps = rmf.getMapsFromPid(" +
                             std::to_string(pid) + ")"))
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

        guard.execString("maps = rmf.getMapsFromPid(" +
                         std::to_string(pid) + ")");
        guard.execString("readable = maps.filterHasPerms(\"r\")");

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

        guard.execString("maps = rmf.getMapsFromPid(" +
                         std::to_string(pid) + ")");
        guard.execString("readable = maps.filterHasPerms(\"r\")");
        guard.execString(
            "chunked = readable.breakIntoChunks(0x1000)");

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

        guard.execString("maps = rmf.getMapsFromPid(" +
                         std::to_string(pid) + ")");
        guard.execString("active = maps.filterActiveRegions(" +
                         std::to_string(pid) + ")");

        results = guard.getLocals()["active"]
                      .cast<rmf::types::MemoryRegionPropertiesVec>();
    }
    tp.stop();

    EXPECT_GT(results.size(), 0);
}

TEST(PythonBindingsTest, findNumericExact_i32)
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

        guard.execString("maps = rmf.getMapsFromPid(" +
                         std::to_string(pid) + ")");
        guard.execString("readable = maps.filterHasPerms(\"r\")");

        guard.execString(R"(
            snapshots = []
            for mrp in readable:
                try:
                    snap = rmf.MemorySnapshot(mrp, )" +
                         std::to_string(pid) + R"()
                    if snap.isValid():
                        snapshots.append(snap)
                except:
                    pass
        )");

        guard.execString("results = []");
        guard.execString(R"(
            for snap in snapshots:
                found = rmf.findNumericExact_i32(snap, )" +
                         std::to_string(targetValue) + R"()
                if found:
                    results.extend(found)
        )");

        results = guard.getLocals()["results"]
                      .cast<rmf::types::MemoryRegionPropertiesVec>();
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

        guard.execString("maps = rmf.getMapsFromPid(" +
                         std::to_string(pid) + ")");
        guard.execString("readable = maps.filterHasPerms(\"r\")");

        guard.execString(R"(
            snapshots = []
            for mrp in readable:
                try:
                    snap = rmf.MemorySnapshot(mrp, )" +
                         std::to_string(pid) + R"()
                    if snap.isValid():
                        snapshots.append(snap)
                except:
                    pass
        )");

        guard.execString("results = []");
        guard.execString(R"(
            for snap in snapshots:
                found = rmf.findNumericExact_i64(snap, )" +
                         std::to_string(targetValue) + R"()
                if found:
                    results.extend(found)
        )");

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

        guard.execString("maps = rmf.getMapsFromPid(" +
                         std::to_string(pid) + ")");
        guard.execString("readable = maps.filterHasPerms(\"r\")");

        guard.execString(R"(
            snapshots = []
            for mrp in readable:
                try:
                    snap = rmf.MemorySnapshot(mrp, )" +
                         std::to_string(pid) + R"()
                    if snap.isValid():
                        snapshots.append(snap)
                except:
                    pass
        )");

        guard.execString("results = []");
        guard.execString(R"(
            for snap in snapshots:
                found = rmf.findNumericExact_f32(snap, )" +
                         std::to_string(targetValue) + R"()
                if found:
                    results.extend(found)
        )");

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

        guard.execString("maps = rmf.getMapsFromPid(" +
                         std::to_string(pid) + ")");
        guard.execString("readable = maps.filterHasPerms(\"r\")");

        guard.execString(R"(
            snapshots = []
            for mrp in readable:
                try:
                    snap = rmf.MemorySnapshot(mrp, )" +
                         std::to_string(pid) + R"()
                    if snap.isValid():
                        snapshots.append(snap)
                except:
                    pass
        )");

        guard.execString("results = []");
        guard.execString(R"(
            import struct
            packed = struct.pack('d', )" +
                         std::to_string(targetValue) + R"()
            val = struct.unpack('q', packed)[0]
            for snap in snapshots:
                found = rmf.findNumericExact_i64(snap, val)
                if found:
                    results.extend(found)
        )");

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

        guard.execString("maps = rmf.getMapsFromPid(" +
                         std::to_string(pid) + ")");
        guard.execString("readable = maps.filterHasPerms(\"r\")");

        guard.execString(R"(
            snapshots = []
            for mrp in readable:
                try:
                    snap = rmf.MemorySnapshot(mrp, )" +
                         std::to_string(pid) + R"()
                    if snap.isValid():
                        snapshots.append(snap)
                except:
                    pass
        )");

        guard.execString("results = []");
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

    EXPECT_EQ(results.size(), 2);
}

TEST(PythonBindingsTest, findChangedRegions)
{
    using namespace rmf::test;

    rmf::types::MemoryRegionPropertiesVec results{};
    rmf::test::testProcess                tp;
    tp.build<rmf::test::staticValueComponent>();
    pid_t pid = tp.run();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    {
        rmf::py::embedPythonScopedGuard guard{};

        guard.execString("maps = rmf.getMapsFromPid(" +
                         std::to_string(pid) + ")");
        guard.execString("readable = maps.filterHasPerms(\"r\")");

        guard.execString(R"(
            mrp = readable[0]
            snap1 = rmf.MemorySnapshot(mrp, )" +
                         std::to_string(pid) + R"()
        )");

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        guard.execString(R"(
            snap2 = rmf.MemorySnapshot(mrp, )" +
                         std::to_string(pid) + R"()
        )");

        guard.execString(
            "results = rmf.findChangedRegions(snap1, snap2, 32)");

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

        guard.execString("maps = rmf.getMapsFromPid(" +
                         std::to_string(pid) + ")");
        guard.execString("readable = maps.filterHasPerms(\"rw\")");

        guard.execString(R"(
            mrp = readable[0]
            snap1 = rmf.MemorySnapshot(mrp, )" +
                         std::to_string(pid) + R"()
        )");

        std::this_thread::sleep_for(std::chrono::seconds(2));

        guard.execString(R"(
            snap2 = rmf.MemorySnapshot(mrp, )" +
                         std::to_string(pid) + R"()
        )");

        guard.execString(
            "results = rmf.findNumericChanged_i32(snap1, snap2, 0)");

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

        guard.execString("maps = rmf.getMapsFromPid(" +
                         std::to_string(pid) + ")");
        guard.execString("readable = maps.filterHasPerms(\"r\")");

        guard.execString(R"(
            mrp = readable[0]
            snap1 = rmf.MemorySnapshot(mrp, )" +
                         std::to_string(pid) + R"()
        )");

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        guard.execString(R"(
            snap2 = rmf.MemorySnapshot(mrp, )" +
                         std::to_string(pid) + R"()
        )");

        guard.execString(
            "results = rmf.findNumericUnchanged_i32(snap1, snap2, "
            "256)");

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

        guard.execString("maps = rmf.getMapsFromPid(" +
                         std::to_string(pid) + ")");
        guard.execString("readable = maps.filterHasPerms(\"r\")");

        guard.execString(R"(
            snapshots = []
            for mrp in readable:
                try:
                    snap = rmf.MemorySnapshot(mrp, )" +
                         std::to_string(pid) + R"()
                    if snap.isValid():
                        snapshots.append(snap)
                except:
                    pass
        )");

        guard.execString("results = []");
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

        guard.execString("maps = rmf.getMapsFromPid(" +
                         std::to_string(pid) + ")");
        guard.execString("readable = maps.filterHasPerms(\"r\")");

        guard.execString(R"(
            addr = readable[0].trueAddress()
            region = readable.getRegionContainingAddress(addr)
        )");

        auto region = guard.getLocals()["region"];
        EXPECT_TRUE(region.is_none() == false);
    }
    tp.stop();
}
