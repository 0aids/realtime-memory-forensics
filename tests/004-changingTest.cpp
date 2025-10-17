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
    size_t iter = 0;

    // Quickly perform the raw check for the changing region
    // (usually UnnamedRegion-3)
    {
        bool   foundChanging = false;
        size_t maxIter       = map.size() - 10;

        for (const auto& props : map)
        {
            MemorySnapshot mp1 = makeSnapshotST(props);
            this_thread::sleep_for(50ms);
            MemorySnapshot mp2 = makeSnapshotST(props);

            BuildJob<RegionPropertiesList> build;
            findChangedRegionsCore(
                build,
                makeMemoryPartitions(mp1.regionProperties, 1).front(),
                mp1, mp2, 8);

            auto changedRegions = build.getResult();
            Log(Message,
                "Number of changed regions in region ["
                    << props.regionName
                    << "] : " << changedRegions.size());

            if (changedRegions.size() > 0)
            {
                foundChanging = true;
                break;
            }
            if (iter >= maxIter)
            {
                break;
            }
            iter++;
        }
        assert(
            foundChanging,
            "There should be a changing region somewhere there...");
    }
    {
        ThreadPool     tp(12);
        MemorySnapshot mp1 = makeSnapshotST(map[iter]);
        this_thread::sleep_for(50ms);
        MemorySnapshot       mp2 = makeSnapshotST(map[iter]);
        RegionPropertiesList changingRegions =
            findChangedRegionsMT(mp1, mp2, tp, 8);
        assert(changingRegions.size() > 0, "There is definitely a changing region here");
    }
    {
        // TODO: Make a method for RegionPropertiesList that allows for creation of
        // a subset of the list

        //     RegionPropertiesList rl;
        //     for (size_t i = 0 ; i < 5; i++)
        //     {
        //         rl.push_back(map[i]);
        //     }
        //     std::vector<MemorySnapshot> mps =makeLotsOfSnapshotsST(rl);
        //     MemorySnapshot& mp = mps[0];
        //     assert(mp[1] == 'E' && mp[2] == 'L' && mp[3] == 'F', "There should be an ELF there...");
        //     cerr << mp[1] << mp[2] << mp[3] << endl;
        // }
        return 0;
    }
}
