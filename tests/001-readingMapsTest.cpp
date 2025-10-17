#include "logs.hpp"
#include "maps.hpp"
#include "tests.hpp"


int main() {
    auto pid = runSampleProcess();
    auto map = readMapsFromPid(pid);
    assert(map.size() > 0, "There should be regions inside the map");

    Log(Message, "Sample of map[0]: \n " << map.front());
}
