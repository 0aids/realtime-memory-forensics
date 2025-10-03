// Testing retrieving region data.
#include "log.hpp"
#include "memory_map.hpp"
#include <csignal>
#include <iostream>
#include "run_test.hpp"
using namespace std;
int main() {
    pid_t pid = runSampleProcess();
    Log(Message, "Sample process pid: " << pid);

    MemoryMap map(pid);
    map.snapshotMaps();
    auto list = map.m_mapSnapshots_l.back();

    for (const auto& item : list) {
        Log(Verbose, "\n" << item);
    }
    kill(pid, SIGTERM);
};
