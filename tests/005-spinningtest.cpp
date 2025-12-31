#include "multi_threading.hpp"
#include "utils.hpp"
#include <functional>
#include <iostream>
#include "logger.hpp"
#include <thread>
#include <chrono>
#include <vector>

#pragma GCC push_options
#pragma GCC optimize("O0")

using namespace std;
using namespace rmf;
constexpr size_t numTasks = 100;
constexpr size_t numLoops = 400000000;
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
                for (size_t i = 0; i < numLoops; i++) {
                }
            };
        taskVec.emplace_back(lambda);
    }
    threadpool.SubmitMultipleTasks(taskVec);
    threadpool.AwaitTasks();
}

#pragma GCC pop_options
