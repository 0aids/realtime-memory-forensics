#include "logs.hpp"
#include "maps.hpp"
#include "tests.hpp"
#include "core.hpp"
#include "core_wrappers.hpp"
#include "snapshots.hpp"

int main()
{
    using namespace std;
    using namespace std::chrono;

    auto pid = runSampleProcess();
    auto map = readMapsFromPid(pid);
    assert(map.size() > 0, "There should be regions inside the map");

    Log(Message, "Sample of map[0]: \n " << map.front());

    {
        MemorySnapshot mp(makeSnapshotCore({.mrp = map[0]}), map[0], steady_clock::now().time_since_epoch());
        assert(mp[1] == 'E' && mp[2] == 'L' && mp[3] == 'F', "There should be an ELF there...");
        cerr << mp[1] << mp[2] << mp[3] << endl;
    }
    {
        auto task = createTask(makeSnapshotCore, {.mrp = map[0]});
        task.packagedTask();
        auto mp= task.result.get();
        assert(mp[1] == 'E' && mp[2] == 'L' && mp[3] == 'F', "There should be an ELF there...");
        cerr << mp[1] << mp[2] << mp[3] << endl;
    }
    {
        TaskThreadPool tp(3);
        auto regions = divideSingleRegion(map[0], 3);
        // Convert a region list into inputs.
        std::vector<CoreInputs> coreInputsVec;
        for (const auto &region : regions ) {
            coreInputsVec.push_back({.mrp=region});
        }

        auto tasks = createMultipleTasks(makeSnapshotCore, coreInputsVec);

        tp.submitMultipleTasks(tasks);
        tp.awaitAllTasks();
        vector<vector<char>> result;

        for (auto &task: tasks) {
            result.push_back(task.result.get());
        }

        auto mp = consolidateNestedVector(result);

        assert(mp[1] == 'E' && mp[2] == 'L' && mp[3] == 'F', "There should be an ELF there...");
        cerr << mp[1] << mp[2] << mp[3] << endl;
    }
    // {
    //     // TODO: Make a method for RegionPropertiesList that allows for creation of
    //     // a subset of the list


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
