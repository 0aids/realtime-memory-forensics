#include <iostream>
#include <vector>
#include <string>
#include <sched.h>
#include "utils.hpp"
#include "rmf.hpp"
#include "logger.hpp"
#include "types.hpp"
#include "operations.hpp"

// Use the namespaces as defined in the reference
using namespace rmf;
using namespace rmf::utils;
using namespace rmf::op;
using namespace rmf::types;

int main() {
    using namespace std;

    // --- 1. Initialization and Inputs ---
    size_t numThreads;
    cout << "Num threads: ";
    cin >> numThreads;

    pid_t pid;
    cout << "Give me the pid: ";
    cin >> pid;

    string location = "/proc/" + to_string(pid) + "/maps";

    cout << "Input loglevel (or empty to continue)(e, w, i, v, d): ";
    string logLevelInput;
    cin >> logLevelInput; // Read string to handle input safely

    // Map Python LogLevel logic to C++ reference global variable setting
    if (!logLevelInput.empty()) {
        char logLevel = logLevelInput[0];
        switch (logLevel) {
            case 'e': rmf::g_logLevel = rmf_Error; break;
            case 'w': rmf::g_logLevel = rmf_Warning; break;
            case 'i': rmf::g_logLevel = rmf_Info; break;
            case 'v': rmf::g_logLevel = rmf_Verbose; break;
            case 'd': rmf::g_logLevel = rmf_Debug; break;
            default:  cout << "No option or invalid, continuing" << endl; break;
        }
    }

    uintptr_t chunkSize = 0x10000;

    // Parse, Filter, and Chunk (Method chaining matching Python logic)
    MemoryRegionPropertiesVec regions = ParseMaps(location, pid)
                                        .FilterPerms("rwp")
                                        .BreakIntoChunks(chunkSize, 0);

    Analyzer anal(numThreads);

    // --- 2. Main Execution Loop ---
    while (true) {
        string a;
        cout << "Enter (1 for changing. 2 for unchanging. 3 for outside window) (or q to quit): ";
        cin >> a;

        // Handle Exit (Python used KeyboardInterrupt, C++ uses 'q' check for grace)
        if (a == "q") {
            cout << "Exiting loop!" << endl;
            break;
        }

        cout << "keyboard interrupt to cancel (Use 'q' to quit in this C++ version)" << endl;

        if (a == "1") {
            // --- Case 1: Changing Detection ---
            cout << "Performing change detection" << endl;
            cout << "Snapshotting 1!" << endl;
            auto snap1 = anal.Execute(MemorySnapshot::Make, regions);

            cout << "Enter to snapshot 2: ";
            string dummy; cin >> dummy; // Pause for user input

            cout << "Snapshotting 2!" << endl;
            auto snap2 = anal.Execute(MemorySnapshot::Make, regions);

            cout << "Finding changed regions" << endl;
            // Execute findChangedRegions with tolerance 4
            auto results = anal.Execute(findChangedRegions, snap1, snap2, 4);
            regions = CompressNestedMrpVec(results);

        } else if (a == "2") {
            // --- Case 2: Unchanged Detection ---
            cout << "Performing unchanged detection" << endl;
            // Matches Python print statement flow
            cout << "Performin change detection" << endl; 

            cout << "Snapshotting 1!" << endl;
            auto snap1 = anal.Execute(MemorySnapshot::Make, regions);

            cout << "Enter to snapshot 2: ";
            string dummy; cin >> dummy;

            cout << "Snapshotting 2!" << endl;
            auto snap2 = anal.Execute(MemorySnapshot::Make, regions);

            cout << "Finding unchanged regions" << endl;
            // Execute findUnchangedRegions with tolerance 4
            std::vector<MemoryRegionPropertiesVec> results = anal.Execute(findUnchangedRegions, snap1, snap2, 4);
            regions = CompressNestedMrpVec(results);

        } else if (a == "3") {
            // --- Case 3: Numeric Determination (Float) ---
            cout << "Performing numeric determination" << endl;
            cout << "Snapshotting 1!" << endl;
            auto snap1 = anal.Execute(MemorySnapshot::Make, regions);

            cout << "finding float!" << endl;
            // Execute findNumeralWithinRange_f32 with range 61.1 - 61.2
            // Note: passing floats explicitly with 'f' suffix
            auto results = anal.Execute(findNumeralWithinRange<float>, snap1, 60.5f, 62.0f);
            regions = CompressNestedMrpVec(results);
        }

        cout << "There are " << regions.size() << " regions left" << endl;
    }

    // --- 3. End of Script ---
    cout << "do whatever now. " << endl;
    // C++ cannot reflect over globals() like Python, so we skip that print.
    
    return 0;
}
