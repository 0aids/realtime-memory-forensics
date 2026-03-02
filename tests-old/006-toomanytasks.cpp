
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
constexpr size_t numTasks = 100000;
int main() {
    uint8_t          numThreads = thread::hardware_concurrency();
    // IMPORTANT: The task vectors MUST be declared BEFORE the threadpool, to prevent
    // data races on destruction.
    vector<Task_t<void>> taskVec;
    taskVec.reserve(numTasks);
    TaskThreadPool_t threadpool(numThreads / 2);

    for (size_t i = 1; i < numTasks + 1; i++) {
        auto lambda = 
            [=](){
                this_thread::sleep_for(chrono::milliseconds(1000 / (i / 10 + 1)));
            };
        taskVec.emplace_back(lambda);
    }
    threadpool.SubmitMultipleTasks(taskVec);
    threadpool.AwaitTasks();
}

#pragma GCC pop_options
