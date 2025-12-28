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

int main() {
    uint8_t          numThreads = thread::hardware_concurrency();
    // IMPORTANT: The task vectors MUST be declared BEFORE the threadpool, to prevent
    // data races on destruction.
    vector<Task_t<std::function<void()>>> taskVec;
    taskVec.reserve(10000);
    TaskThreadPool_t threadpool(numThreads / 2);

    for (size_t i = 0; i < 10000; i++) {
        auto lambda = 
            [i](){
                rmf_Log(rmf_Info, "I am the " << i << "th lambda");
            };
        taskVec.emplace_back(lambda);
    }

    threadpool.SubmitMultipleTasks(taskVec);
    threadpool.AwaitTasks();
    rmf_Log(rmf_Info, "Done!");
}
