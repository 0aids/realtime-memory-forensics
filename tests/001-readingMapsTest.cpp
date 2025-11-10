#include "utils/logs.hpp"
#include "data/maps.hpp"
#include "tests.hpp"

using namespace rmf::tests;

int main() {
    auto pid = runSampleProcess();
    auto map = readMapsFromPid(pid);
    rmf_assert(map.size() > 0, "There should be regions inside the map");

    rmf_Log(rmf_Message, "Sample of map[0]: \n " << map.front());
}
