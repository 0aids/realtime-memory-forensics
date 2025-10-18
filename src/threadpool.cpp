#include "threadpool.hpp"
#include <pthread.h>
#include "logs.hpp"
#include "snapshots.hpp"

std::vector<MemoryPartition>
makeMemoryPartitions(MemoryRegionProperties properties,
                     size_t                 numParts)
{
    std::vector<MemoryPartition> partitions;

    // Align everything to the 64th bit.
    for (size_t i = 0; i < numParts - 1; i++)
    {
        partitions.push_back({
            .relativeRegionStart = i *
                (properties.relativeRegionSize / (numParts * 8)) * 8,
            .size =
                8 * (properties.relativeRegionSize / (numParts * 8)),
        });
    }

    uintptr_t lastStart = (numParts - 1) *
        (properties.relativeRegionSize / (numParts * 8)) * 8;
    partitions.push_back({
        .relativeRegionStart = lastStart,
        .size = properties.relativeRegionSize - lastStart,
    });

    return partitions;
}

std::vector<VectorPartition>
makeVectorPartitionsFromRegionPropertiesList(RegionPropertiesList& rl,
                                             size_t numParts)
{
    uintptr_t totalRegionSize = 0;

    for (const auto& p : rl)
    {
        totalRegionSize += p.relativeRegionSize;
    }

    uintptr_t averageSize = totalRegionSize / numParts;

    std::vector<VectorPartition> parts;
    parts.reserve(numParts);

    size_t                       currentIndex = 0;
    for (size_t i = 0; i < numParts; i++)
    {
        uintptr_t                    currentMemorySize  = 0;
        size_t size          = 0;
        size_t startingIndex = currentIndex;
        // The last index is guaranteed to be done properly?
        // It should be.
        if (numParts - 1 == i) {
            parts.push_back({startingIndex, rl.size() - startingIndex});
            continue;
        }
        while (currentMemorySize + rl[currentIndex+1].relativeRegionSize < averageSize || size == 0)
        {
            currentMemorySize += rl[currentIndex++].relativeRegionSize;
            size++;
        }
        parts.push_back({startingIndex, size});

    }

    return parts;
}

std::vector<VectorPartition>
makeVectorPartitionsFromSnapshotsList(const std::vector<MemorySnapshot> snaps, size_t numParts) {
    RegionPropertiesList rl;
    for (const auto &snap : snaps) {
        rl.push_back(snap.regionProperties);
    }

    return makeVectorPartitionsFromRegionPropertiesList(rl, numParts);
}

void ThreadPool::submitTask(ThreadPoolTask task)
{
    // The fucking print statements go haywire because the thread
    // starts before everything is pushed back...
    // Also the string is needed otherwise we don't have the correct lifetime.
    using namespace std::chrono;
    if (m_threadsList.size() >= m_numThreads)
    {
        Log(Warning, "Attempted to use more threads than allocated in constructor");
        return;
    }
    m_threadsList.push_back(
        {std::thread(
            [task](){
                // For better print statements ig.
                std::this_thread::sleep_for(500us);
                task();
            }
        ),
         std::to_string(m_threadsList.size() + 1)});

    pthread_setname_np(m_threadsList.back().first.native_handle(),
                       m_threadsList.back().second.c_str());
}

void ThreadPool::joinTasks()
{
    if (m_threadsList.size() == 0) return;
    for (auto it = m_threadsList.end() - 1;
         it != m_threadsList.begin() - 1; it--)
    {
        it->first.join();
        Log(Debug, "Joined thread: " << it->second);
        m_threadsList.pop_back();
    }
    return;
}
