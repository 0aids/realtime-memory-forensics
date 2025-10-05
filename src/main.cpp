#include "run_test.hpp"
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

    pid_t                 pid        = std::stoi(argv[1]);
    size_t                largestInd = 0;
    RegionPropertiesList* filtered   = new RegionPropertiesList();
    {
        MemoryMap map(pid);
        auto      mapsnap = map.snapshotMaps();
        *filtered         = mapsnap.getRegionsWithPermissions("rwp");
        cout << "Number filtered: " << filtered->size() << endl;
        for (size_t i = 1; i < filtered->size(); i++)
        {
            if ((*filtered)[largestInd].parentRegionSize <
                (*filtered)[i].parentRegionSize)
            {
                largestInd = i;
            }
        }
    }
    cout << "largest ind: " << largestInd << endl;
    std::vector<MemoryRegionProperties> thing;
    std::vector<MemoryRegionProperties> singlething;
    {
        MemoryRegion m((*filtered)[largestInd], pid);
        delete filtered;
        m.snapshot();
        auto m1 = m.getLastSnapshot();
        cout << m1->size() << endl;
        std::string ha;
        cout << "Waiting for input: " << endl;
        cin >> ha;
        m.snapshot();
        auto m2 = m.getLastSnapshot();

        cout << m2->size() << endl;
        thing  = m1->findChangedRegions(*m2, 64);
        auto t = m1->findOf("print");
        cout << "Number of prints: " << t.size() << endl;
    }
    cout << "number of changed regions: " << thing.size() << endl;
    // std::vector<MemoryRegion> mr;
    // for (const auto& memprop : thing) {
    //     mr.push_back(MemoryRegion(memprop, pid));
    //     mr.back().snapshot();
    // }
    // std::string ha;
    // cout << "Waiting for another input" << endl;
    // cin >> ha;
    //
    // int count = 0;
    // for (auto& reg : mr) {
    //     reg.snapshot();
    //     count +=
    //         reg.m_snapshots_l[0]
    //             ->findUnchangedRegions(*(reg.m_snapshots_l[1]), 64)
    //             .size();
    // }
    // cout << "Number of unchanged regions: " << count;
    // and then have to fucking repeat this again and again until I find my position.
    // That is going to be fucking insane.
}
