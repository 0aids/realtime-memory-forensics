#include "rmf.hpp"
#include "types.hpp"
#include "utils.hpp"
#include <iterator>
#include <vector>
#include "multi_threading.hpp"
#include <thread>
#include "logger.hpp"
#include "test_helpers.hpp"
#include "operations.hpp"
using namespace std;
using namespace rmf::utils;
using namespace rmf::types;
using namespace rmf::op;
using namespace rmf::test;

int main()
{
    rmf::g_logLevel = rmf_Info;
    size_t numThreads = thread::hardware_concurrency() / 2;
    pid_t  samplePid =
        rmf::test::forkTestFunction(rmf::test::changingProcess1);
    {
        auto maps = rmf::utils::ParseMaps(PidToMapsString(samplePid),
                                          samplePid)
                        .BreakIntoChunks(0x100000, 0);
        rmf_Log(rmf_Info, "Number of regions: " << maps.size());
        this_thread::sleep_for(2s);
        rmf_Assert(maps.size() > 1,
                   "There should be more than 1 memory regions...");
        auto analyzer = rmf::Analyzer(numThreads);

        auto snap1 = analyzer.Execute(MemorySnapshot::Make, maps);
        rmf_Log(rmf_Info, "Number of snapshots: " << snap1.size());
        rmf_Assert(
            snap1.size() > 1,
            "There should be more than 1 snapshotted region...");

        auto snap2 = analyzer.Execute(MemorySnapshot::Make, maps);
        rmf_Log(rmf_Info, "Number of snapshots: " << snap2.size());
        rmf_Assert(
            snap2.size() > 1,
            "There should be more than 1 snapshotted region...");

        auto resultsHolder =
            analyzer.Execute(findChangedRegions, snap1, snap2, 4);
        maps.clear();
        for (auto& rr : resultsHolder)
        {
            for (auto& r : rr)
            {
                maps.push_back(r);
            }
        }
        rmf_Log(rmf_Info,
                "Number of changed regions: " << maps.size());
        rmf_Assert(maps.size() > 1,
                   "There should be more than 1 changed region. ");
    }
    {
        auto maps =
            FilterHasPerms(rmf::utils::ParseMaps(
                               PidToMapsString(samplePid), samplePid),
                           "r")
                .BreakIntoChunks(0x1000, 0);
        rmf_Log(rmf_Info, "Number of regions: " << maps.size());
        this_thread::sleep_for(2s);
        rmf_Assert(maps.size() > 1,
                   "There should be more than 1 memory regions...");
        auto analyzer = rmf::Analyzer(numThreads);

        auto snap1 = analyzer.Execute(MemorySnapshot::Make, maps);
        rmf_Log(rmf_Info, "Number of snapshots: " << snap1.size());
        rmf_Assert(
            snap1.size() > 1,
            "There should be more than 1 snapshotted region...");

        auto snap2 = analyzer.Execute(MemorySnapshot::Make, maps);
        rmf_Log(rmf_Info, "Number of snapshots: " << snap2.size());
        rmf_Assert(
            snap2.size() > 1,
            "There should be more than 1 snapshotted region...");

        auto resultsHolder =
            analyzer.Execute(findChangedRegions, snap1, snap2, 4);
        maps.clear();
        for (auto& rr : resultsHolder)
        {
            for (auto& r : rr)
            {
                maps.push_back(r);
            }
        }
        rmf_Log(rmf_Info,
                "Number of changed regions: " << maps.size());
        rmf_Assert(maps.size() > 1,
                   "There should be more than 1 changed region. ");
    }
}
