
#include "core_wrappers.hpp"
#include "logs.hpp"
#include "maps.hpp"
#include "core.hpp"
#include "tests.hpp"
#include "snapshots.hpp"
#include <chrono>

int main()
{
    using namespace std;
    using namespace std::chrono;

    auto pid = runSampleProcess();
    this_thread::sleep_for(500ms);
    auto map = readMapsFromPid(pid);
    assert(map.size() > 0, "There should be regions inside the map");

    Log(Message, "Sample of map[0]: \n " << map.front());
    size_t iter = 0;

    // Quickly perform the raw check for the changing region
    {
        Log(Message,
            "Checking if there is a changing region of memory!");
        bool   foundChanging = false;
        size_t maxIter       = map.size();

        for (const auto& props : map)
        {
            // if (props.regionName != "UnnamedRegion-3") continue;
            MemorySnapshot snap1(
                makeSnapshotCore({.mrp = props}), props,
                steady_clock::now().time_since_epoch());
            this_thread::sleep_for(10ms);
            MemorySnapshot snap2(
                makeSnapshotCore({.mrp = props}), props,
                steady_clock::now().time_since_epoch());

            CoreInputs cInputs  = {.mrp   = props,
                                   .snap1 = snap1.asSpan(),
                                   .snap2 = snap2.asSpan()};
            auto changedRegions = findChangedRegionsCore(cInputs, 8);

            Log(Message,
                "Number of changed regions in region ["
                    << props.regionName
                    << "] : " << changedRegions.size());

            if (changedRegions.size() > 0)
            {
                foundChanging = true;
                Log(Message,
                    "Changed region: " << changedRegions.front());
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
    // {
    //     // TODO: Make a method for RegionPropertiesList that allows for creation of
    //     // a subset of the list

    //     RegionPropertiesList rl;
    //     for (size_t i = 0 ; i < map.size() - 10; i++)
    //     {
    //         rl.push_back(map[i]);
    //     }
    //     std::vector<MemorySnapshot> mps1 = makeLotsOfSnapshotsST(rl);
    //     const MemorySnapshot& mp1 = mps1[0];
    //     assert(mp1[1] == 'E' && mp1[2] == 'L' && mp1[3] == 'F', "There should be an ELF there...");
    //     this_thread::sleep_for(10ms);
    //     std::vector<MemorySnapshot> mps2 = makeLotsOfSnapshotsST(rl);
    //     const MemorySnapshot& mp2 = mps2[0];
    //     assert(mp2[1] == 'E' && mp2[2] == 'L' && mp2[3] == 'F', "There should be an ELF there...");
    //     ThreadPool tp(10);
    //     auto result = findChangedRegionsRegionPoolMT(
    //         mps1,
    //         mps2,
    //         tp,
    //         8
    //     );
    //     assert(result.size() > 1, "There should be a changing region after region pool multi threading...");
    // }
    return 0;
}


