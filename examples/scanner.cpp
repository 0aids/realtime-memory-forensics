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
                    .FilterActiveRegions(pid);
    auto lambda = [&maps, &pid, &matchString](auto analyzer) mutable
    {
        auto start     = chrono::steady_clock::now();
        auto snapshots = analyzer.Execute(
            rmf::types::MemorySnapshot::Make, maps, pid);
        auto results =
            analyzer
                .Execute(rmf::op::findString, snapshots, matchString)
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
}
