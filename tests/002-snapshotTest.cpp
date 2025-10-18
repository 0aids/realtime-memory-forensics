#include "logs.hpp"
#include "maps.hpp"
#include "tests.hpp"
#include "snapshots.hpp"
#include "threadpool.hpp"
#include <chrono>

int main()
{
    using namespace std;
    using namespace std::chrono;

    auto pid = runSampleProcess();
    auto map = readMapsFromPid(pid);
    assert(map.size() > 0, "There should be regions inside the map");

    Log(Message, "Sample of map[0]: \n " << map.front());

    MemorySnapshot::Builder build(map.front());

    MemoryPartition part = {
        0,
        map.front().relativeRegionSize
    };

    makeSnapshotCore(build, part);
    MemorySnapshot mp = build.build();
    assert(mp.size() > 0, "There should be data that has been read");
    assert(mp.regionProperties.parentRegionSize == map.front().parentRegionSize, "The region sizes should be the same");
    assert(mp[1] == 'E' && mp[2] == 'L' && mp[3] == 'F', "There should be an ELF there...");
    cerr << mp[1] << mp[2] << mp[3] << endl;
    return 0;
}
