#include "multi_threading.hpp"
#include "utils.hpp"
#include <functional>
#include <iostream>
#include "logger.hpp"
#include <thread>
#include <chrono>
#include <vector>

using namespace std;
using namespace rmf;
std::atomic<size_t> globalCounter{0};
constexpr size_t numTasks = 100000;


int main() {
    uint8_t          numThreads = thread::hardware_concurrency();
    // IMPORTANT: The task vectors MUST be declared BEFORE the threadpool, to prevent
    // data races on destruction.
    vector<Task_t<void>> taskVec;
    taskVec.reserve(numTasks);
    TaskThreadPool_t threadpool(numThreads / 2);

    for (size_t i = 0; i < numTasks; i++) {
        auto lambda = 
            [](){
                globalCounter++;
            };
        taskVec.emplace_back(lambda);
    }

    threadpool.SubmitMultipleTasks(taskVec);
    threadpool.AwaitTasks();
    this_thread::sleep_for(100ms);
    rmf_Log(rmf_Info, "numTasks: " << numTasks << ", global counter: " << globalCounter);
    if (globalCounter != numTasks) {
        rmf_Log(rmf_Error, "Tasks were dropped or duplicated!");
        return 1;
    }
    rmf_Log(rmf_Info, "Done!");
    return 0;
}
