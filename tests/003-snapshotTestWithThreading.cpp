#include "logs.hpp"
#include "maps.hpp"
#include "tests.hpp"
#include "snapshots.hpp"
#include "threadpool.hpp"

int main()
{
    using namespace std;
    using namespace std::chrono;

    auto pid = runSampleProcess();
    auto map = readMapsFromPid(pid);
    assert(map.size() > 0, "There should be regions inside the map");

    Log(Message, "Sample of map[0]: \n " << map.front());

    {
        MemorySnapshot mp = makeSnapshotST(map.front());
        assert(mp[1] == 'E' && mp[2] == 'L' && mp[3] == 'F', "There should be an ELF there...");
        cerr << mp[1] << mp[2] << mp[3] << endl;
    }
    {
        ThreadPool tp(12);
        MemorySnapshot mp = makeSnapshotMT(map.front(), tp);
        assert(mp[1] == 'E' && mp[2] == 'L' && mp[3] == 'F', "There should be an ELF there...");
        cerr << mp[1] << mp[2] << mp[3] << endl;
    }
    {
        // TODO: Make a method for RegionPropertiesList that allows for creation of
        // a subset of the list


        RegionPropertiesList rl;
        for (size_t i = 0 ; i < 5; i++) 
        {
            rl.push_back(map[i]);
        }
        std::vector<MemorySnapshot> mps =makeLotsOfSnapshotsST(rl);
        MemorySnapshot& mp = mps[0];
        assert(mp[1] == 'E' && mp[2] == 'L' && mp[3] == 'F', "There should be an ELF there...");
        cerr << mp[1] << mp[2] << mp[3] << endl;
    }
    return 0;
}
