#include "utils/logs.hpp"
#include "data/maps.hpp"
#include "tests.hpp"
#include "data/snapshots.hpp"
#include "backends/core.hpp"
#include <chrono>

int main()
{
    using namespace std;
    using namespace std::chrono;
    using namespace rmf::data;
    using namespace rmf::backends::core;
    using namespace rmf::tests;

    auto pid = runSampleProcess();
    auto map = readMapsFromPid(pid);
    rmf_assert(map.size() > 0, "There should be regions inside the map");

    rmf_Log(rmf_Message, "Sample of map[0]: \n " << map.front());
    MemorySnapshot mp(makeSnapshotCore({.mrp = map[0]}));
    rmf_assert(mp[1] == 'E' && mp[2] == 'L' && mp[3] == 'F', "There should be an ELF there...");
    cerr << mp[1] << mp[2] << mp[3] << endl;

    rmf_assert(mp.size() > 0, "There should be data that has been read");
    rmf_assert(mp.regionProperties.parentRegionSize == map.front().parentRegionSize, "The region sizes should be the same");
    rmf_assert(mp[1] == 'E' && mp[2] == 'L' && mp[3] == 'F', "There should be an ELF there...");
    cerr << mp[1] << mp[2] << mp[3] << endl;
    return 0;
}
