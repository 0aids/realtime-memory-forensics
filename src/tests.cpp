#include <sys/resource.h>
#include "data/refreshable_snapshots.hpp"
#include "utils/logs.hpp"
#include "tests.hpp"
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <thread>
#include <cstdlib>
#include <list>
#include <unistd.h>
#include <sys/prctl.h>
#include <csignal>

namespace rmf::tests {
using namespace rmf::data;
using namespace rmf::backends::mt;
__attribute__((optimize("O0"))) void changingMapMemorySampleProcess()
{
    using namespace std;
    string thing;
    thing = "this is a thing!!!";
    this_thread::sleep_for(1000ms);
    while (true)
    {
        thing.reserve(10000000);
        this_thread::sleep_for(1000ms);
        thing.shrink_to_fit();
    }
}

pid_t runChangingMapProcess()
{
    //
    pid_t pid = fork();
    if (pid < 0)
    {
        rmf_Log(rmf_Error, "Forking failed");
        exit(EXIT_FAILURE);
    }
    if (pid == 0)
    {
        if (prctl(PR_SET_PDEATHSIG, SIGKILL) == -1)
        {
            perror("prctl failed");
            exit(EXIT_FAILURE);
        }
        changingMapMemorySampleProcess();
        _exit(127);
    }
    rmf_Log(rmf_Debug, "Child pid: " << pid);
    return pid;
}

__attribute__((optimize("O0"))) void sampleProcess()
{
    using namespace std;

    volatile char    extrashit[] = "This is some extra shit";
    volatile char    i[]         = "ALL HAIL THE!!!";
    volatile char    j[]         = "A FASTER!!!";
    volatile double* d           = new double(5);
    volatile string* randomthing = new string("small string");
    volatile double* doubleVal   = new double(150);
    volatile auto    list = new std::list<char>{'a', 'b', 'c', 'd'};
    cerr << "randomthing address: " << std::hex << std::showbase
         << &randomthing << endl;
    cerr << "linked list address: " << std::hex << std::showbase
         << &list << endl;

    string* otherrandomshit = new string(
        " Lorem ipsum dolor sit amet, consectetur adipiscing elit. "
        "Mauris "
        "commodo, felis non semper fermentum, lorem velit dictum "
        "risus, id "
        "rutrum ipsum nisi eget ex. Curabitur ullamcorper "
        "consectetur venenatis. "
        "Vestibulum ultrices iaculis arcu quis suscipit. Duis "
        "tincidunt "
        "venenatis fermentum. Fusce vehicula consequat erat, quis "
        "bibendum arcu "
        "iaculis a. In pharetra ligula massa. Pellentesque nisi ex, "
        "vehicula at "
        "est nec, molestie egestas enim. Integer eleifend eros in "
        "leo tempor, at "
        "gravida lectus hendrerit. Aenean at ligula eu leo "
        "pellentesque "
        "elementum. Morbi sed mattis metus, sit amet ultrices orci. "
        "Sed euismod, "
        "ex sed hendrerit sagittis, nibh sapien lobortis nisl, ut "
        "dignissim "
        "tellus nulla ut ex. Phasellus eu convallis odio. Morbi "
        "dignissim "
        "bibendum lorem, quis sollicitudin est vehicula facilisis. "
        "Cras "
        "venenatis in enim vitae efficitur. Sed lacinia viverra "
        "turpis elementum "
        "maximus. Nam sit amet rhoncus nisl. Etiam dapibus ligula "
        "vel faucibus "
        "dapibus. Curabitur commodo ante non arcu efficitur "
        "hendrerit. Ut "
        "consectetur scelerisque nulla, id molestie metus commodo "
        "ac. Etiam a "
        "efficitur orci, ac bibendum velit. In lacinia viverra ex id "
        "euismod. "
        "Sed nec dolor vel purus rutrum pharetra ac non est. Nullam "
        "lobortis "
        "rutrum ligula, nec sagittis sem bibendum nec. Vivamus "
        "sagittis ligula "
        "enim, et tempus ex porttitor in. Cras mattis ex in nibh "
        "tempus, non "
        "rhoncus risus iaculis. Sed tincidunt elementum ligula in "
        "convallis. Nam "
        "faucibus ipsum in sagittis dignissim. Nam a elit mauris. "
        "Pellentesque "
        "auctor erat sed pellentesque tempor. Etiam at congue ante. "
        "Pellentesque ");
    size_t           iterations = 0;
    volatile size_t* dumb       = new size_t(0);
    while (true)
    {
        iterations++;
        this_thread::sleep_for(1ms);
        if (!(iterations % 500))
        {
            i[0] = (i[0] - 64) % 26 + 65;
        }
        j[0] = (i[0] - 64) % 26 + 65;
        *d += 0.5;
        *dumb += 1;
    }
    delete randomthing;
    delete otherrandomshit;
}

bool checkPtraceScope()
{
    const std::string_view ptrace_scope =
        "/proc/sys/kernel/yama/ptrace_scope";
    std::ifstream inputFile(ptrace_scope.data());
    std::string   line;
    if (!std::getline(inputFile, line))
    {
        rmf_Log(rmf_Error, "Failed to open '" << ptrace_scope << "'");
        return false;
    }
    if (line != "0")
    {
        rmf_Log(rmf_Error,
            "File: '" << ptrace_scope
                      << "' is not 0, unable to continue");
        return false;
    }
    return true;
}

pid_t runSampleProcess()
{
    pid_t pid = fork();
    if (pid < 0)
    {
        rmf_Log(rmf_Error, "Forking failed");
        exit(EXIT_FAILURE);
    }
    if (pid == 0)
    {
        if (prctl(PR_SET_PDEATHSIG, SIGKILL) == -1)
        {
            perror("prctl failed");
            exit(EXIT_FAILURE);
        }
        int ret =
            setpriority(PRIO_PROCESS, 0,
                        -20); // higher priority (lower nice value)
        if (!ret)
        {
            rmf_Log(rmf_Warning, "Failed to change forked process priority!");
        }
        sampleProcess();
        _exit(127);
    }
    rmf_Log(rmf_Debug, "Child pid: " << pid);
    return pid;
}

std::pair<std::shared_ptr<RefreshableSnapshot>,
          std::shared_ptr<RefreshableSnapshot>>
getSampleRefreshableSnapshots(pid_t sampleProcessPID)
{
    using namespace std;
    RegionPropertiesList map =
        readMapsFromPid(sampleProcessPID).filterRegionsByPerms("rwp");

    RegionPropertiesList result;
    {
        rmf_Log(rmf_Message, "Attempting TASK splitting WITH MT!");
        size_t numThreads = 10;
        map               = breakIntoRegionChunks(map, 0);
        QueuedThreadPool tp(numThreads);

        auto cinputs = consolidateIntoCoreInput({.mrpVec = map});
        auto snapTasks1 =
            createMultipleTasks(makeSnapshotCore, cinputs);
        auto snapTasks2 =
            createMultipleTasks(makeSnapshotCore, cinputs);

        tp.submitMultipleTasks(snapTasks1);
        tp.awaitAllTasks();
        this_thread::sleep_for(10ms);
        tp.submitMultipleTasks(snapTasks2);
        tp.awaitAllTasks();

        vector<MemorySnapshot>     snapshotsList1;
        vector<MemorySnapshot>     snapshotsList2;
        vector<MemorySnapshotSpan> spansList1;
        vector<MemorySnapshotSpan> spansList2;
        snapshotsList1.reserve(snapTasks1.size());
        snapshotsList2.reserve(snapTasks2.size());
        spansList1.reserve(snapTasks1.size());
        spansList2.reserve(snapTasks2.size());

        for (size_t j = 0; j < snapTasks1.size(); j++)
        {
            snapshotsList1.push_back(
                MemorySnapshot(snapTasks1[j].result.get()));
            spansList1.push_back(
                snapshotsList1.back().asSnapshotSpan());
            snapshotsList2.push_back(
                MemorySnapshot(snapTasks2[j].result.get()));
            spansList2.push_back(
                snapshotsList2.back().asSnapshotSpan());
        }
        cinputs = consolidateIntoCoreInput({.mrpVec   = map,
                                            .snap1Vec = spansList1,
                                            .snap2Vec = spansList2});

        auto tasks =
            createMultipleTasks(findChangedRegionsCore, cinputs, 8);
        tp.submitMultipleTasks(tasks);
        tp.awaitAllTasks();
        result = consolidateNestedTaskResults(tasks);
    }
    return {make_shared<RefreshableSnapshot>(
                breakIntoRegionChunks(map, 0).front()),
            make_shared<RefreshableSnapshot>(result.front())};
}
};
