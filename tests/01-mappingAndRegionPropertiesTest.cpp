// Testing retrieving region data.
#include "log.hpp"
#include "memory_map.hpp"
#include <iostream>
#include "run_test.hpp"
using namespace std;
int main() {
    pid_t pid = runSampleProcess();
    Log(Message, "Sample process pid: " << pid);

    MemoryMap map(pid);
    map.readMaps();
    auto list = map.getPropertiesList();

    for (const auto& item : list) {
        Log(Verbose, "\n" << item);
    }
};
