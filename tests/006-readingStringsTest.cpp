#include "core_wrappers.hpp"
#include "logs.hpp"
#include "maps.hpp"
#include "core.hpp"
#include "tests.hpp"
#include "snapshots.hpp"
#include <chrono>
#include <thread>

int main()
{
    using namespace std;
    using namespace std::chrono;

    auto pid = runSampleProcess();
    this_thread::sleep_for(500ms);
    auto map = readMapsFromPid(pid);
    assert(map.size() > 0, "There should be regions inside the map");

    Log(Message, "Sample of map[0]: \n " << map.front());
    {
        bool found = false;
        for (const auto& r : map)
        {
            auto snap =
                MemorySnapshot(makeSnapshotCore({.mrp = r}));
            auto res = findStringCore(
                {.snap1 = snap.asSnapshotSpan()},
                " Lorem ipsum dolor sit amet, consectetur adipiscing "
                "elit. ");
            if (res.size() > 0)
            {
                found  = true;
                auto s = snap.asSnapshotSpan().subspan(
                    res[0].relativeRegionStart,
                    res[0].relativeRegionSize);
                for (size_t i = 0; i < s.size(); i++)
                {
                    cerr << s[i];
                }
                cerr << endl;
                auto snap = MemorySnapshot(
                    makeSnapshotCore({.mrp = res[0]}));
                for (size_t i = 0; i < snap.size(); i++)
                {
                    cerr << snap[i];
                }
                cerr << endl;
                auto newRes =
                    findStringCore({.snap1 = snap}, "Lorem ipsum");
                assert(newRes.size() > 0,
                       "There should be a lorem ipsum here!");
                break;
            }
        }
        assert(found, "There is definitely a lorem ipsum in there!");
    }
    {
        Log(Message, "attempting pooled search!");
        size_t numThreads = 5;
        auto newMap = breakIntoRegionChunks(map, 0);
        newMap.resize(newMap.size() - 3);
        auto inputs = consolidateIntoCoreInput({.mrpVec = newMap});
        auto tasks = createMultipleTasks(makeSnapshotCore, inputs);
        QueuedThreadPool tp(numThreads);

        // for (auto &task : tasks) {
        //     task.packagedTask();
        // }

        tp.submitMultipleTasks(tasks);
        tp.awaitAllTasks();
        std::vector<MemorySnapshot> snapshots;
        std::vector<MemorySnapshotSpan> spans;

        for (size_t i = 0; i < tasks.size(); i++) {
            snapshots.emplace_back(tasks[i].result.get());
            spans.push_back(snapshots.back().asSnapshotSpan());
        }
        inputs = consolidateIntoCoreInput({.mrpVec = newMap, .snap1Vec = spans});

        auto newTasks = createMultipleTasks(findStringCore, inputs, "Lorem ipsum");
        // for (auto &task : newTasks) {
        //     task.packagedTask();
        // }
        tp.submitMultipleTasks(newTasks);
        tp.awaitAllTasks();

        auto res = consolidateNestedTaskResults(newTasks);
        assert(res.size() > 0, "There is definitely a lorem ipsum in there!");
        if (res.size() > 0)
        {
            auto snap = MemorySnapshot(
                makeSnapshotCore({.mrp = res[0]}));
            for (size_t i = 0; i < snap.size(); i++)
            {
                cerr << snap[i];
            }
            cerr << endl;
            auto newRes =
                findStringCore({.snap1 = snap.asSnapshotSpan()}, "Lorem ipsum");
            assert(newRes.size() > 0,
                   "There should be a lorem ipsum here!");
        }
    }
}
