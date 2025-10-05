// Testing retrieving region data.
#include "log.hpp"
#include "memory_region.hpp"
#include "memory_map.hpp"
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <thread>
#include "run_test.hpp"
using namespace std;
int main()
{
    pid_t pid = runChangingMapProcess();
    Log(Message, "Sample process pid: " << pid);

    MemoryMap map(pid);
    map.snapshotMaps();
    this_thread::sleep_for(1000ms);
    map.snapshotMaps();
    this_thread::sleep_for(1000ms);
    map.snapshotMaps();

    Log_f(Warning, "Not warning, but snapshots 0 and 1");
    map.m_mapSnapshots_l.compareSnapshots(0, 1);
    Log_f(Warning, "");
    Log_f(Warning, "Not warning, but snapshots 1 and 2");
    map.m_mapSnapshots_l.compareSnapshots(1, 2);
    kill(pid, SIGTERM);
}
