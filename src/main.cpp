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
    Logger::m_logLevel = Debug;
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
    RegionPropertiesList changedRegions =
        mmap.m_mapSnapshots_l[0].getRegionsWithPermissions("rwp");
    std::string                         ha;
    std::vector<MemoryRegionProperties> regionProperties =
        changedRegions.getRegionsWithSubstrName("UnnamedRegion-");
    // for (const auto& region : changedRegions)
    // {
    //     if (region.regionSize > 1 << 23)
    //     {
    //         regionProperties.push_back(std::move(region));
    //     }
    // }
    cout << "Number of big regions: " << regionProperties.size()
         << endl;
    for (const auto& bigregion : regionProperties)
    {
        std::vector<MemoryRegionProperties> newRegionProperties = {
            bigregion};
        cout << "Before the inner loop: waiting for input" << endl;
        ha = "";
        cin >> ha;
        while (newRegionProperties.size() > 0 && ha != "found" &&
               ha != "skip")
        {
            std::vector<MemoryRegion> mrList;
            for (const auto& region : newRegionProperties)
            {
                mrList.push_back(MemoryRegion(region, pid));
                if (!mrList.back().snapshot())
                {
                    mrList.pop_back();
                    continue;
                }
            }
            cout << "Current size: " << newRegionProperties.size()
                 << endl;
            cout << "Waiting for input for change: " << endl;
            cin >> ha;
            if (ha == "found" || ha == "skip")
                break;
            newRegionProperties.clear();
            for (auto& memRegion : mrList)
            {
                if (!memRegion.snapshot())
                {
                    continue;
                };
                auto changes =
                    memRegion.m_snapshots_l[0]->findChangedRegions(
                        *memRegion.m_snapshots_l[1], 8);

                std::copy(std::make_move_iterator(changes.begin()),
                          std::make_move_iterator(changes.end()),
                          std::back_inserter(newRegionProperties));
            }
            cout << "Current size: " << newRegionProperties.size()
                 << endl;
        }
        if (ha == "found")
            break;
    }
}
