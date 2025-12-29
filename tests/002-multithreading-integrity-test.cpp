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
    g_logLevel = rmf_Verbose;
    const size_t bufferSize = 100 * 1024 * (2 << 11);
    std::vector<uint8_t> buffer(bufferSize);
    auto startG = std::chrono::steady_clock::now();
    std::generate(buffer.begin(), buffer.end(), [](){return rand() % 256;});
    auto endG = std::chrono::steady_clock::now();

    // --- Single-threaded Checksum ---
    auto startST = std::chrono::steady_clock::now();
    
    uint64_t groundTruth = 0;
    for (auto b : buffer) groundTruth += b;
    
    auto endST = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::milli> diffST = endST - startST;

    rmf_Log(rmf_Info, "Ground truth value: " << groundTruth);

    const size_t numChunks = 1024;
    const size_t chunkSize = bufferSize / numChunks;
    rmf_Log(rmf_Info, "chunkSize: " << chunkSize << "\tnumChunks: " << numChunks);

    std::vector<Task_t<uint64_t>> tasks;
    tasks.reserve(numChunks);

    for (size_t i = 0; i < numChunks; ++i) {
        uint8_t* ptr = buffer.data() + (i * chunkSize);
        size_t currentChunkSize = (i == numChunks - 1) ? (bufferSize - (i * chunkSize)) : chunkSize;
        tasks.emplace_back(checksumTask, ptr, currentChunkSize);
    }

    // --- Multi-threaded Checksum ---
    auto startMT = std::chrono::steady_clock::now();

    pool.SubmitMultipleTasks(tasks);
    pool.AwaitTasks();

    auto endMT = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::milli> diffMT = endMT - startMT;

    // Aggregate Results
    uint64_t parallelSum = 0;
    for (auto& t : tasks) {
        parallelSum += t.getFuture().get();
    }

    rmf_Log(rmf_Info, "Generation of data took: " << chrono::duration_cast<chrono::milliseconds>(endG - startG));
    rmf_Log(rmf_Info, "Single-threaded checksum took: " << diffST.count() << " ms");
    rmf_Log(rmf_Info, "Multi-threaded checksum took: " << diffMT.count() << " ms");

    // Validation
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
