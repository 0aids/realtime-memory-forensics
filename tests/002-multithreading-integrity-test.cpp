#include "multi_threading.hpp"
#include "utils.hpp"
#include <iostream>
#include "logger.hpp"
#include <thread>
#include <chrono>
#include <vector>

using namespace std;
using namespace rmf;

// Simulation of your task function
uint64_t checksumTask(const uint8_t* start, size_t length) {
    uint64_t localSum = 0;
    for (size_t i = 0; i < length; ++i) {
        localSum += start[i];
    }
    rmf_Log(rmf_Debug, "Start: " << static_cast<const void*>(start) << " Found sum: " << localSum);
    return localSum;
}

bool runIntegrityTest(TaskThreadPool_t& pool) {
    const size_t bufferSize = 100 * 1024 * 128;
    std::vector<uint8_t> buffer(bufferSize);
    std::generate(buffer.begin(), buffer.end(), [](){return rand() % 256;});

    uint64_t groundTruth = 0;
    for (auto b : buffer) groundTruth += b;
    rmf_Log(rmf_Info, "Ground truth value: " << groundTruth);

    const size_t numChunks = 1024;
    const size_t chunkSize = bufferSize / numChunks;
    rmf_Log(rmf_Info, "chunkSize: " << chunkSize << "\tnumChunks: " << numChunks);

    std::vector<Task_t<std::function<decltype(checksumTask)>>> tasks;
    struct inputs {
        const uint8_t* ptr;
        const size_t currentChunkSize;
    };
    std::vector<inputs> inputVec;

    // don't get fucking moved.
    inputVec.reserve(numChunks);

    tasks.reserve(numChunks);

    for (size_t i = 0; i < numChunks; ++i) {
        // Handle the last chunk which might be slightly larger if bufferSize isn't perfectly divisible
        inputVec.emplace_back(buffer.data() + (i * chunkSize),(i == numChunks - 1) ? (bufferSize - (i * chunkSize)) : chunkSize);

        // Assuming your Task_t or Submit takes a lambda/function and arguments
        rmf_Log(rmf_Info, "Setting start: " << static_cast<const void*>(inputVec.back().ptr) << "\tChunkSize: " << inputVec.back().currentChunkSize);
        tasks.emplace_back(checksumTask, inputVec.back().ptr, inputVec.back().currentChunkSize);
    }

    // 4. Submit and Wait
    pool.SubmitMultipleTasks(tasks);
    pool.AwaitTasks();

    // 5. Aggregate Results
    uint64_t parallelSum = 0;
    for (auto& t : tasks) {
        parallelSum += t.future.get();
    }
    // 6. Validation
    if (parallelSum == groundTruth) {
        rmf_Log(rmf_Info, "SUCCESS: Checksum matches! (" << parallelSum << ")");
        return true;
    } else {
        rmf_Log(rmf_Error, "FAILURE: Checksum mismatch!");
        rmf_Log(rmf_Error, "Expected: " << groundTruth << " | Got: " << parallelSum);
        return false;
    }
}

int main() {
    srand(time(NULL));
    uint8_t          numThreads = thread::hardware_concurrency();
    TaskThreadPool_t threadpool(numThreads / 2);
    if (runIntegrityTest(threadpool)) return 0;
    else return 1;
}
