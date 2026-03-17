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
    if (argc != 2)
    {
        cout << "Incorrect arguments! Expecting PID as second "
                "argument!"
             << endl;
    }
    pid_t pid = std::stoul(argv[1]);

    rmf::g_logLevel = rmf_Warning;

    auto maps = rmf::utils::getMapsFromPid(pid)
                    .FilterHasPerms("r")
                    .BreakIntoChunks(0x10000);
    auto lambda = [&maps, &pid](auto analyzer) mutable
    {
        auto start     = chrono::steady_clock::now();
        auto snapshots = analyzer.Execute(
            rmf::types::MemorySnapshot::Make, maps, pid);
        auto results = rmf::utils::flattenArray(
            analyzer.Execute(rmf::op::findString, snapshots,
                             std::string("Snipingcamper2017")));
        cout << "Results length: " << results.size() << endl;
        return chrono::steady_clock::now() - start;
    };

    std::chrono::duration<double> time1;
    std::chrono::duration<double> time2;
    {
        rmf::Analyzer analyzer(thread::hardware_concurrency() - 1);

        time1 = lambda(analyzer);
    }
    // {
    //     rmf::Analyzer analyzer(1);
    //     time2 = lambda(analyzer);
    // }
    cout << "Time taken " << thread::hardware_concurrency() - 1
         << " threads: " << time1 << endl;
    // cout << "Time taken 1 thread: " << time2 << endl;
}
