#include <iostream>
#include <sched.h>
#include "utils.hpp"
#include "rmf.hpp"
#include "logger.hpp"
#include "types.hpp"
#include "operations.hpp"
#include <string>

using namespace rmf;
using namespace rmf::utils;
using namespace rmf::op;
using namespace rmf::types;


int main() {
    using namespace std;
    cout << "Num threads: ";
    size_t numThreads;
    cin >> numThreads;
    cout << "Pid: ";
    pid_t pid;
    cin >> pid;

    std::string location = "/proc/" + to_string(pid) + "/maps";

    cout << "Input loglevel (or empty to continue)(e, w, i, v, d): ";
    char logLevel;
    cin >> logLevel;
    switch (logLevel)
    {
    case 'e':
        rmf::g_logLevel = rmf_Error;
        break;
    case 'w':
        rmf::g_logLevel = rmf_Warning;
        break;
    case 'i':
        rmf::g_logLevel = rmf_Info;
        break;
    case 'v':
        rmf::g_logLevel = rmf_Verbose;
        break;
    case 'd':
        rmf::g_logLevel = rmf_Debug;
        break;
    default:
        cout << "No option or invalid, continuing";
    }

    uintptr_t chunkSize = 0x10000;
    MemoryRegionPropertiesVec regions = ParseMaps(location, pid).FilterPerms("rwp").BreakIntoChunks(chunkSize, 0);

    auto anal = Analyzer(numThreads);
    std::string a;
    cout << "Input 'q' to quit (or anything else to continue): ";
    cin >> a;
    while (a != "q")
    {
        cout << "Performing changing region loop" << endl;
        {
            cout << "Snapshotting snap1" << endl;
            auto snap1 = anal.Execute(MemorySnapshot::Make, regions);
            cout << "Input 'q' to quit (or anything else to continue): ";
            cin >> a;
            if (a == "q") break;
            cout << "Snapshotting snap2" << endl;
            auto snap2 = anal.Execute(MemorySnapshot::Make, regions);
            cout << "Finding changed regions" << endl;
            auto rrr = anal.Execute(findChangedRegions, snap1, snap2, 4);
            cout << "Consolidating regions" << endl;
            regions.clear();
            for (auto &rr:rrr) {
                for (auto &r: rr) {
                    regions.push_back(r);
                }
            }
            cout << "There are " << regions.size() << " regions left" << endl;
        }
        cout << "Perfomrming non-changing loop" << endl;
        cout << "Input 'q' to quit (or anything else to continue): ";
        cin >> a;
        if (a == "q") break;
        {
            cout << "Snapshotting snap1" << endl;
            auto snap1 = anal.Execute(MemorySnapshot::Make, regions);
            cout << "Input 'q' to quit (or anything else to continue): ";
            cin >> a;
            if (a == "q") break;
            cout << "Snapshotting snap2" << endl;
            auto snap2 = anal.Execute(MemorySnapshot::Make, regions);
            cout << "Finding unchanged regions" << endl;
            auto rrr = anal.Execute(findUnchangedRegions, snap1, snap2, 4);
            cout << "Consolidating regions" << endl;
            regions.clear();
            for (auto &rr:rrr) {
                for (auto &r: rr) {
                    regions.push_back(r);
                }
            }
            cout << "There are " << regions.size() << " regions left" << endl;
        }    
        cout << "About to start changing region loop" << endl;
        cout << "Input 'q' to quit (or anything else to continue): ";
        cin >> a;
        if (a == "q") break;
    }

}
