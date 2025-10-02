#include "run_test.hpp"
#include <thread>
#include <chrono>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include "log.hpp"
#include "memory_map.hpp"
#include "memory_region.hpp"
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <vector>

void stopProcess(pid_t pid) {
    if (ptrace(PTRACE_ATTACH, pid, nullptr, nullptr) == -1) {
        perror("PTRACE_ATTACH");
        exit(EXIT_FAILURE);
    }

    // 2. Wait for the process to stop (it receives a SIGSTOP)
    int status;
    waitpid(pid, &status, 0);
    if (WIFEXITED(status)) {
        std::cerr << "Process exited before we could analyze."
                  << std::endl;
        exit(EXIT_FAILURE);
    }

    std::cout << "Process " << pid
              << " stopped. Starting memory analysis..." << std::endl;
}

void releaseProcess(pid_t pid) {
    // 4. Detach from the process, allowing it to resume
    if (ptrace(PTRACE_DETACH, pid, nullptr, nullptr) == -1) {
        perror("PTRACE_DETACH");
    }
}

int main(int argc, char* argv[]) {
    using namespace std;
    if (argc != 2) {
        Log(Error,
            "Incorrect arguments. Should be 'mem-anal (PID)', where "
            "pid is the pid of the process.");
        exit(EXIT_FAILURE);
    }
    if (!checkPtraceScope()) {
        Log(Error,
            "Ptrace scope would not allow for memory reading. "
            "\nUse:\n\techo 0 | sudo tee "
            "/proc/sys/kernel/yama/ptrace_scope");
        exit(EXIT_FAILURE);
    }

    pid_t     pid = std::stoi(argv[1]);
    MemoryMap map(pid);
    map.readMaps();
    auto                 rwp = map.getRegionsWithPermissions("rwp");
    RegionPropertiesList readWritePrivateRegions;
    readWritePrivateRegions.push_back(rwp[3]);
    readWritePrivateRegions.push_back(rwp[4]);

    vector<MemoryRegion*>  memRegions1;
    vector<RegionSnapshot> snapshots1;
    for (const auto& regionP : readWritePrivateRegions) {
        if (map.hasChanged()) {
            Log(Error, "Map has changed!");
            // return 0;
        }
        memRegions1.push_back(new MemoryRegion(regionP, pid));
        memRegions1.back()->snapshot();
        snapshots1.push_back(memRegions1.back()->getLastSnapshot());
    }

    this_thread::sleep_for(10000ms);

    map.readMaps();
    rwp = map.getRegionsWithPermissions("rwp");
    readWritePrivateRegions.clear();
    readWritePrivateRegions.push_back(rwp[3]);
    readWritePrivateRegions.push_back(rwp[4]);

    vector<MemoryRegion*>  memRegions2;
    vector<RegionSnapshot> snapshots2;
    for (const auto& regionP : readWritePrivateRegions) {
        memRegions2.push_back(new MemoryRegion(regionP, pid));
        memRegions2.back()->snapshot();
        snapshots1.push_back(memRegions2.back()->getLastSnapshot());
    }
    int total = 0;
    for (int i = 0; i < (int)snapshots1.size(); i++) {
        if (i >= (int)snapshots2.size())
            break;

        auto diff =
            snapshots1[i].findChangedRegions(snapshots2[i], 8);
        total += diff.size();
    }

    cout << "Total differing regions: " << total << endl;
}
