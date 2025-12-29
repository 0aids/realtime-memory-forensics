#include "multi_threading.hpp"
#include "utils.hpp"
#include <iostream>
#include "logger.hpp"
#include <thread>
#include <chrono>
#include <vector>

using namespace std;
using namespace rmf;
static constexpr size_t numTasks = 1 << 12;

#pragma GCC push_options
#pragma GCC optimize("O0")

static std::atomic<size_t> testFunc0Count = 1;
int         testFunc0()
{
    int                        sum   = 0;
    for (size_t i = 0; i < 1 << 14; i++)
    {
        sum += i;
    }
    // if (testFunc0Count % (numTasks / 100) == 0)
    //     rmf_Log(rmf_Debug,
    //             "Task: " << testFunc0Count << " out of " << numTasks
    //                      << " complete!");
    // testFunc0Count++;
    return sum;
}

#pragma GCC pop_options

int main()
{
    uint8_t          numThreads = thread::hardware_concurrency();
    {
        vector<Task_t<int>> taskVec;
        taskVec.reserve(numTasks);
        TaskThreadPool_t threadpool(numThreads / 2);
        for (size_t i = 0; i < numTasks; i++)
        {
            taskVec.emplace_back(testFunc0);
        }

        auto curTime = std::chrono::steady_clock::now();
        threadpool.SubmitMultipleTasks(taskVec);
        threadpool.AwaitTasks();
        rmf_Log(rmf_Debug, "Value of thing: " << taskVec[0].getFuture().get());
        auto endTime = std::chrono::steady_clock::now();
        rmf_Log(rmf_Info,
                "Time taken for multi-threaded: "
                    << chrono::duration_cast<chrono::milliseconds>(
                           endTime - curTime));
        this_thread::sleep_for(2s);
    }
    // Single thread to ensure speedup actaully occurs.
    {
        testFunc0Count = 0;
        vector<Task_t<int>> taskVec;
        taskVec.reserve(numTasks);
        for (size_t i = 0; i < numTasks; i++)
        {
            taskVec.emplace_back(testFunc0);
        }
        auto curTime = std::chrono::steady_clock::now();
        for (auto& task: taskVec) {
            task.getPackagedTask()();
        }
        rmf_Log(rmf_Debug, "Value of thing: " << taskVec[0].getFuture().get());
        auto endTime = std::chrono::steady_clock::now();
        rmf_Log(rmf_Info,
                "Time taken for single-threaded: "
                    << chrono::duration_cast<chrono::milliseconds>(
                           endTime - curTime));
    }
}
