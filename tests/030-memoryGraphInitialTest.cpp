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
    rmf::g_logLevel   = rmf_Verbose;
    size_t numThreads = thread::hardware_concurrency() / 2;
    pid_t  samplePid =
        rmf::test::forkTestFunction(rmf::test::changingProcess1);
    auto ogmaps =
        rmf::utils::ParseMaps(PidToMapsString(samplePid), samplePid)
            .FilterPerms("rwp")
            .BreakIntoChunks(0x100000, 0);
    auto maps = ogmaps;
    rmf_Log(rmf_Info, "Number of regions: " << maps.size());
    this_thread::sleep_for(2s);
    rmf_Assert(maps.size() > 1,
               "There should be more than 1 memory regions...");
    auto analyzer = rmf::Analyzer(numThreads);

    {
        auto snap1 = analyzer.Execute(MemorySnapshot::Make, maps);
        rmf_Log(rmf_Info, "Number of snapshots: " << snap1.size());
        rmf_Assert(
            snap1.size() > 1,
            "There should be more than 1 snapshotted region...");
        auto resultsHolder =
            analyzer.Execute(findNumeralExact<double>, snap1, 150);
        maps.clear();
        for (auto& rr : resultsHolder)
        {
            for (auto& r : rr)
            {
                maps.push_back(r);
            }
        }
    }
    rmf_Log(
        rmf_Info,
        "Number of regions containing <double>150: " << maps.size());
    rmf_Assert(
        maps.size() == 1,
        "There should be exactly 1 region containing <double>150");
    auto        doublemrp = maps[0];
    MemoryGraph mg;

    {
        auto trans = mg.StartCreateMemoryRegionTransaction();
        trans.mrp  = maps[0];
        trans.name = "double 150 holder";
        mg.ProcessTransaction(trans);
        maps = ogmaps;
        rmf_Log(rmf_Info, "Mrp: " << trans.mrp.toString());
        rmf_Assert(mg.GetMemoryRegionMapSize() == 1,
                   "There should only be 1 region"
                       << mg.GetMemoryRegionMapSize());
    }

    {
        auto snap1 = analyzer.Execute(MemorySnapshot::Make, maps);
        rmf_Log(rmf_Info, "Number of snapshots: " << snap1.size());
        rmf_Assert(
            snap1.size() > 1,
            "There should be more than 1 snapshotted region...");
        auto resultsHolder =
            analyzer.Execute(findPointersToRegion, snap1, doublemrp);
        maps.clear();
        for (auto& rr : resultsHolder)
        {
            for (auto& r : rr)
            {
                maps.push_back(r);
            }
        }
    }
    rmf_Assert(
        maps.size() == 1,
        "There should be exactly 1 region containing <double>150");
    rmf_Log(rmf_Info,
            "Number of pointers to that region: " << maps.size());
    auto pointerToDouble = maps[0];
    {
        auto trans       = mg.StartCreateMemoryLinkTransaction();
        trans.policy     = MemoryLinkPolicy::CreateSourceTarget;
        trans.sourceAddr = doublemrp.TrueAddress();
        trans.targetAddr = pointerToDouble.TrueAddress();
        trans.name       = "150 linker";
        mg.ProcessTransaction(trans);
    }
    rmf_Assert(mg.GetMemoryRegionLinkMapSize() == 1,
               "There should be only one link, num Links: "
                   << mg.GetMemoryRegionLinkMapSize());
    rmf_Assert(mg.GetMemoryRegionMapSize() == 2,
               "There should only be 2 regions, one created via the "
               "link policy, num regions: "
                   << mg.GetMemoryRegionMapSize());
    rmf_Log(rmf_Info, "");
    rmf_Log(rmf_Info, "Printing link details");
    for (auto [id, link] : mg.m_memoryLinkMap)
    {
        rmf_Log(rmf_Info, link.toString());
    }
    rmf_Log(rmf_Info, "");
    rmf_Log(rmf_Info, "Printing Region Details");
    for (auto [id, map] : mg.m_memoryRegionMap)
    {
        rmf_Log(rmf_Info, map.toStringDetailed(true));
    }
}
