#include "run_test.hpp"
#include <iterator>
#include <ranges>
#include <thread>
#include <chrono>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include "log.hpp"
#include "memory_map.hpp"
#include "memory_region.hpp"
#include "region_properties.hpp"
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <vector>

void stopProcess(pid_t pid)
{
    if (ptrace(PTRACE_ATTACH, pid, nullptr, nullptr) == -1)
    {
        perror("PTRACE_ATTACH");
        exit(EXIT_FAILURE);
    }

    // 2. Wait for the process to stop (it receives a SIGSTOP)
    int status;
    waitpid(pid, &status, 0);
    if (WIFEXITED(status))
    {
        std::cerr << "Process exited before we could analyze."
                  << std::endl;
        exit(EXIT_FAILURE);
    }

    std::cout << "Process " << pid
              << " stopped. Starting memory analysis..." << std::endl;
}

void releaseProcess(pid_t pid)
{
    // 4. Detach from the process, allowing it to resume
    if (ptrace(PTRACE_DETACH, pid, nullptr, nullptr) == -1)
    {
        perror("PTRACE_DETACH");
    }
}

int main(int argc, char* argv[])
{
    using namespace std;
    Logger::m_logLevel = Message;
    if (argc != 2)
    {
        Log(Error,
            "Incorrect arguments. Should be 'mem-anal (PID)', where "
            "pid is the pid of the process.");
        exit(EXIT_FAILURE);
    }
    if (!checkPtraceScope())
    {
        Log(Error,
            "Ptrace scope would not allow for memory reading. "
            "\nUse:\n\techo 0 | sudo tee "
            "/proc/sys/kernel/yama/ptrace_scope");
        exit(EXIT_FAILURE);
    }

    pid_t     pid = std::stoi(argv[1]);

    MemoryMap mmap(pid);
    mmap.snapshotMaps();
    std::vector<MemoryRegionProperties> changedRegions{
        mmap.m_mapSnapshots_l.getLargestRegionFromLastSnapshot()};
    std::string                         ha;
    std::vector<MemoryRegionProperties> result;

    {
        MemoryRegion mr(changedRegions[0], pid);
        if (!mr.snapshot())
        {
            return 1;
        }
        auto lastSnap = mr.getLastSnapshot();
        result        = lastSnap->findDoubleLike(100, 200);
    }

    cout << "Results size: " << result.size() << endl;

    while (result.size() > 3)
    {
        std::vector<MemoryRegion> mrList;
        for (const auto& region : result)
        {
            mrList.push_back(MemoryRegion(std::move(region), pid));
            if (!mrList.back().snapshot())
            {
                mrList.pop_back();
                continue;
            }
        }
        result.clear();
        for (auto& memreg : mrList)
        {
            auto lastSnap = memreg.getLastSnapshot();

            auto newResults = lastSnap->findDoubleLike(100, 200);

            std::copy(std::make_move_iterator(newResults.begin()),
                      std::make_move_iterator(newResults.end()),
                      std::back_inserter(result));
        }
        cout << "Current result size: " << result.size() << endl;
        this_thread::sleep_for(1s);
    }
}
