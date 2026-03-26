#include "logger.hpp"
#include "operations.hpp"
#include "types.hpp"
#include "utils.hpp"
#include <rmf.hpp>
#include <iostream>
#include <sched.h>
#include <chrono>

using namespace std;
int main(int argc, const char** argv)
{
    if (argc != 3)
    {
        cout << "Incorrect arguments! Expecting PID as second "
                "argument AND string to match for as third"
             << endl;
    }
    pid_t       pid         = std::stoul(argv[1]);
    std::string matchString = argv[2];

    rmf::g_logLevel = rmf_Warning;

    auto maps = rmf::utils::getMapsFromPid(pid)
                    .FilterHasPerms("r")
                    .FilterActiveRegions(pid)
                    .BreakIntoChunks(0x10000000);
    cout << "Num maps: " << maps.size() << endl;
    auto snapAndFind =
        [pid](const rmf::types::MemoryRegionProperties& mrp,
              const std::string&                        str)
    {
        auto snap = rmf::types::MemorySnapshot::Make(mrp, pid);
        return rmf::op::findString(snap, str);
    };
    auto lambda = [&snapAndFind, &maps, &pid,
                   &matchString](auto analyzer) mutable
    {
        auto start = chrono::steady_clock::now();
        auto results =
            analyzer.Execute(snapAndFind, maps, matchString)
                .flatten();
        cout << "Results length: " << results.size() << endl;
        cout << "Results type: " << typeid(results).name() << endl;
        return chrono::steady_clock::now() - start;
    };

    std::chrono::duration<double> time1;
    {
        rmf::Analyzer analyzer(thread::hardware_concurrency() - 1);

        time1 = lambda(analyzer);
    }
    cout << "Time taken " << thread::hardware_concurrency() - 1
         << " threads: " << time1 << endl;

    // Trying again with no analyzer.
    maps = rmf::utils::getMapsFromPid(pid)
               .FilterHasPerms("r")
               .FilterActiveRegions(pid)
               .BreakIntoChunks(0x10000000);
    auto start = chrono::steady_clock::now();
    rmf::types::MemoryRegionPropertiesVec results;
    for (const auto& map : maps)
    {
        auto snap   = rmf::types::MemorySnapshot::Make(map, pid);
        auto result = rmf::op::findString(snap, matchString);
        for (auto&& r : result)
        {
            results.emplace_back(std::move(r));
        }
    }
    std::chrono::duration<double> timeTaken =
        chrono::steady_clock::now() - start;
    cout << "Time taken S.T: " << timeTaken << endl;
    cout << "Num results: " << results.size() << endl;
}
