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

class BatcherMemoryScanningTest : public ::testing::Test
{
  protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(BatcherMemoryScanningTest, FindString)
{
    using namespace rmf::test;

    rmf::test::testProcess tp;
    tp.build<rmf::test::staticStringTestComponent>();
    pid_t pid = tp.run();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    pid_t captured_pid = pid;
    {
        rmf::py::embedPythonScopedGuard guard{};

        guard.execString(std::format(R"(
pid = {}
maps = rmf.getMapsFromPid(pid).filterActiveRegions(pid).filterHasPerms('r')

num_results = 0
for mrp in maps:
    snap = rmf.MakeSnapshot(mrp, pid)
    if snap.isValid():
        found = rmf.findString(snap, "I am a small string")
        if found:
            num_results += len(found)
)",
                                     captured_pid));

        int num_results =
            guard.getGlobals()["num_results"].cast<int>();
        EXPECT_GE(num_results, 1);
    }

    tp.stop();
}

TEST_F(BatcherMemoryScanningTest, FindNumericExact_i32)
{
    using namespace rmf::test;

    constexpr int32_t      targetValue = 0x99998888;

    rmf::test::testProcess tp;
    tp.build<rmf::test::staticValueComponent>(targetValue);
    pid_t pid = tp.run();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    pid_t captured_pid = pid;
    {
        rmf::py::embedPythonScopedGuard guard{};

        guard.execString(std::format(R"(
pid = {}
maps = rmf.getMapsFromPid(pid).filterActiveRegions(pid).filterHasPerms('r')

num_results = 0
for mrp in maps:
    snap = rmf.MakeSnapshot(mrp, pid)
    if snap.isValid():
        found = rmf.findNumericExact_i32(snap, {})
        if found:
            num_results += len(found)
)",
                                     captured_pid, targetValue));

        int num_results =
            guard.getGlobals()["num_results"].cast<int>();
        EXPECT_GE(num_results, 1);
    }

    tp.stop();
}

TEST_F(BatcherMemoryScanningTest, FindNumericExact_i64)
{
    using namespace rmf::test;

    constexpr int64_t      targetValue = 0xfedcba9876543210;

    rmf::test::testProcess tp;
    tp.build<rmf::test::staticValueComponent>(0, targetValue);
    pid_t pid = tp.run();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    pid_t captured_pid = pid;
    {
        rmf::py::embedPythonScopedGuard guard{};

        guard.execString(std::format(R"(
pid = {}
maps = rmf.getMapsFromPid(pid).filterActiveRegions(pid).filterHasPerms('r')

num_results = 0
for mrp in maps:
    snap = rmf.MakeSnapshot(mrp, pid)
    if snap.isValid():
        found = rmf.findNumericExact_i64(snap, {})
        if found:
            num_results += len(found)
)",
                                     captured_pid, targetValue));

        int num_results =
            guard.getGlobals()["num_results"].cast<int>();
        EXPECT_GE(num_results, 1);
    }

    tp.stop();
}

TEST_F(BatcherMemoryScanningTest, FindNumericExact_f32)
{
    using namespace rmf::test;

    constexpr float        targetValue = 3.14159f;

    rmf::test::testProcess tp;
    tp.build<rmf::test::staticValueComponent>(0, 0, targetValue, 0.0);
    pid_t pid = tp.run();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    pid_t captured_pid = pid;
    {
        rmf::py::embedPythonScopedGuard guard{};

        guard.execString(std::format(R"(
pid = {}
maps = rmf.getMapsFromPid(pid).filterActiveRegions(pid).filterHasPerms('r')

num_results = 0
for mrp in maps:
    snap = rmf.MakeSnapshot(mrp, pid)
    if snap.isValid():
        found = rmf.findNumericExact_f32(snap, {})
        if found:
            num_results += len(found)
)",
                                     captured_pid, targetValue));

        int num_results =
            guard.getGlobals()["num_results"].cast<int>();
        EXPECT_GE(num_results, 1);
    }

    tp.stop();
}

TEST_F(BatcherMemoryScanningTest, FindNumericExact_f64)
{
    using namespace rmf::test;

    constexpr double       targetValue = 2.718281828;

    rmf::test::testProcess tp;
    tp.build<rmf::test::staticValueComponent>(0, 0, 0.0f,
                                              targetValue);
    pid_t pid = tp.run();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    pid_t captured_pid = pid;
    {
        rmf::py::embedPythonScopedGuard guard{};

        guard.execString(std::format(R"(
import struct
pid = {}
packed = struct.pack('d', {})
val = struct.unpack('q', packed)[0]

maps = rmf.getMapsFromPid(pid).filterActiveRegions(pid).filterHasPerms('r')

num_results = 0
for mrp in maps:
    snap = rmf.MakeSnapshot(mrp, pid)
    if snap.isValid():
        found = rmf.findNumericExact_i64(snap, val)
        if found:
            num_results += len(found)
)",
                                     captured_pid, targetValue));

        int num_results =
            guard.getGlobals()["num_results"].cast<int>();
        EXPECT_GE(num_results, 1);
    }

    tp.stop();
}

TEST_F(BatcherMemoryScanningTest, FindNumericExact_u8)
{
    using namespace rmf::test;

    constexpr uint32_t     targetValue = 0xF0000000;

    rmf::test::testProcess tp;
    tp.build<rmf::test::staticValueComponent>(targetValue);
    pid_t pid = tp.run();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    pid_t captured_pid = pid;
    {
        rmf::py::embedPythonScopedGuard guard{};

        guard.execString(std::format(R"(
pid = {}
maps = rmf.getMapsFromPid(pid).filterActiveRegions(pid).filterHasPerms('r')

num_results = 0
for mrp in maps:
    snap = rmf.MakeSnapshot(mrp, pid)
    if snap.isValid():
        found = rmf.findNumericExact_u32(snap, {})
        if found:
            num_results += len(found)
)",
                                     captured_pid, targetValue));

        int num_results =
            guard.getGlobals()["num_results"].cast<int>();
        EXPECT_GE(num_results, 1);
    }

    tp.stop();
}

TEST_F(BatcherMemoryScanningTest, FindNumericChanged_i32)
{
    using namespace rmf::test;

    rmf::test::testProcess tp;
    tp.build<rmf::test::incrementingIntComponent>(0, 1);
    pid_t pid = tp.run();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    pid_t captured_pid = pid;
    {
        rmf::py::embedPythonScopedGuard guard{};

        guard.execString(std::format(R"(
pid = {}
maps = rmf.getMapsFromPid(pid).filterActiveRegions(pid).filterHasPerms('rw')

snap1_list = []
for mrp in maps:
    snap = rmf.MakeSnapshot(mrp, pid)
    if snap.isValid():
        snap1_list.append(snap)
)",
                                     captured_pid));

        std::this_thread::sleep_for(std::chrono::seconds(1));

        guard.execString(std::format(R"(
snap2_list = []
for mrp in maps:
    snap = rmf.MakeSnapshot(mrp, pid)
    if snap.isValid():
        snap2_list.append(snap)

num_results = 0
for s1, s2 in zip(snap1_list, snap2_list):
    found = rmf.findNumericChanged_i32(s1, s2, 0)
    if found:
        num_results += len(found)
)",
                                     captured_pid));

        int num_results =
            guard.getGlobals()["num_results"].cast<int>();
        EXPECT_GE(num_results, 1);
    }

    tp.stop();
}

TEST_F(BatcherMemoryScanningTest, FindNumericUnchanged_i32)
{
    using namespace rmf::test;

    rmf::test::testProcess tp;
    tp.build<rmf::test::staticValueComponent>();
    pid_t pid = tp.run();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    pid_t captured_pid = pid;
    {
        rmf::py::embedPythonScopedGuard guard{};

        guard.execString(std::format(R"(
pid = {}
maps = rmf.getMapsFromPid(pid).filterActiveRegions(pid).filterHasPerms('r')

snap1_list = []
for mrp in maps:
    snap = rmf.MakeSnapshot(mrp, pid)
    if snap.isValid():
        snap1_list.append(snap)
)",
                                     captured_pid));

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        guard.execString(std::format(R"(
snap2_list = []
for mrp in maps:
    snap = rmf.MakeSnapshot(mrp, pid)
    if snap.isValid():
        snap2_list.append(snap)

num_results = 0
for s1, s2 in zip(snap1_list, snap2_list):
    found = rmf.findNumericUnchanged_i32(s1, s2, 256)
    if found:
        num_results += len(found)
)",
                                     captured_pid));

        int num_results =
            guard.getGlobals()["num_results"].cast<int>();
        EXPECT_GE(num_results, 1);
    }

    tp.stop();
}

TEST_F(BatcherMemoryScanningTest, FindNumericWithinRange_i32)
{
    using namespace rmf::test;

    constexpr int32_t      targetValue = 0x12345678;

    rmf::test::testProcess tp;
    tp.build<rmf::test::staticValueComponent>(targetValue);
    pid_t pid = tp.run();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    pid_t captured_pid = pid;
    {
        rmf::py::embedPythonScopedGuard guard{};

        guard.execString(std::format(R"(
pid = {}
maps = rmf.getMapsFromPid(pid).filterActiveRegions(pid).filterHasPerms('r')

num_results = 0
for mrp in maps:
    snap = rmf.MakeSnapshot(mrp, pid)
    if snap.isValid():
        found = rmf.findNumericWithinRange_i32(snap, 0x12340000, 0x12350000)
        if found:
            num_results += len(found)
)",
                                     captured_pid));

        int num_results =
            guard.getGlobals()["num_results"].cast<int>();
        EXPECT_GE(num_results, 1);
    }

    tp.stop();
}

TEST_F(BatcherMemoryScanningTest, FindChangedRegions)
{
    using namespace rmf::test;

    rmf::test::testProcess tp;
    tp.build<rmf::test::incrementingIntComponent>();
    pid_t pid = tp.run();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    pid_t captured_pid = pid;
    {
        rmf::py::embedPythonScopedGuard guard{};

        guard.execString(std::format(R"(
pid = {}
maps = rmf.getMapsFromPid(pid).filterActiveRegions(pid).filterHasPerms('rw')

snap1_list = []
for mrp in maps:
    snap = rmf.MakeSnapshot(mrp, pid)
    if snap.isValid():
        snap1_list.append(snap)
)",
                                     captured_pid));

        std::this_thread::sleep_for(std::chrono::seconds(2));

        guard.execString(std::format(R"(
snap2_list = []
for mrp in maps:
    snap = rmf.MakeSnapshot(mrp, pid)
    if snap.isValid():
        snap2_list.append(snap)

num_results = 0
for s1, s2 in zip(snap1_list, snap2_list):
    found = rmf.findChangedRegions(s1, s2, 32)
    if found:
        num_results += len(found)
)",
                                     captured_pid));

        int num_results =
            guard.getGlobals()["num_results"].cast<int>();
        EXPECT_GE(num_results, 1);
    }

    tp.stop();
}

TEST_F(BatcherMemoryScanningTest, GetMapsFromPid)
{
    using namespace rmf::test;

    rmf::types::MemoryRegionPropertiesVec results{};
    rmf::test::testProcess                tp;
    tp.build<rmf::test::staticValueComponent>();
    pid_t pid = tp.run();

    {
        rmf::py::embedPythonScopedGuard guard{};

        if (guard.execString(
                std::format("maps = rmf.getMapsFromPid({})", pid)))
        {
            results =
                guard.getGlobals()["maps"]
                    .cast<rmf::types::MemoryRegionPropertiesVec>();
        }
    }

    tp.stop();

    EXPECT_GT(results.size(), 0);
}

TEST_F(BatcherMemoryScanningTest, FilterHasPerms)
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

        results = guard.getGlobals()["readable"]
                      .cast<rmf::types::MemoryRegionPropertiesVec>();
    }

    tp.stop();

    EXPECT_GT(results.size(), 0);
}

TEST_F(BatcherMemoryScanningTest, BreakIntoChunks)
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

        results = guard.getGlobals()["chunked"]
                      .cast<rmf::types::MemoryRegionPropertiesVec>();
    }

    tp.stop();

    EXPECT_GT(results.size(), 0);
}

TEST_F(BatcherMemoryScanningTest, FilterActiveRegions)
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

        results = guard.getGlobals()["active"]
                      .cast<rmf::types::MemoryRegionPropertiesVec>();
    }

    tp.stop();

    EXPECT_GT(results.size(), 0);
}

TEST_F(BatcherMemoryScanningTest, GetRegionContainingAddress)
{
    using namespace rmf::test;

    rmf::test::testProcess tp;
    tp.build<rmf::test::staticValueComponent>();
    pid_t pid = tp.run();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    pid_t captured_pid = pid;
    {
        rmf::py::embedPythonScopedGuard guard{};

        guard.execString(std::format("maps = rmf.getMapsFromPid({})",
                                     captured_pid));
        guard.execString(R"(readable = maps.filterHasPerms("r"))");

        guard.execString(R"(
addr = readable[0].trueAddress()
region = readable.getRegionContainingAddress(addr)
is_none = region is None
)");

        auto is_none = guard.getGlobals()["is_none"].cast<bool>();
        EXPECT_FALSE(is_none);
    }

    tp.stop();
}
