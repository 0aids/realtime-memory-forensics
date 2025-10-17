// Random ass idea: A resizable, circular array, which allows popping from both ends quickly?
#ifndef threadpool_hpp_INCLUDED
#define threadpool_hpp_INCLUDED
#include <thread>
#include <vector>
#include "maps.hpp"
#include <utility>
#include <unistd.h>
#include <functional>
#include "concepts.hpp"

struct MemoryPartition
{
    // Relative to the buffer that this struct is interacting with.
    const uintptr_t relativeRegionStart;
    const uintptr_t size;
};

// Makes memory partitions, aligned to 64 bits.
std::vector<MemoryPartition>
makeMemoryPartitions(MemoryRegionProperties properties,
                     size_t                 numParts);

struct VectorPartition
{
    const size_t startIndex;
    const size_t size; // Not inclusive
};

std::vector<VectorPartition>
makeVectorPartitionsFromRegionPropertiesList(RegionPropertiesList& rl,
                                             size_t numParts);

template <Buildable ResultType>
struct BuildJob
{
    // private:
    ResultType::Builder build;

  public:
    // The result is moved.
    inline ResultType getResult()
    {
        return build.build();
    }

    // Forward any arguments required to build.
    template <typename... Args>
    BuildJob(Args&&... args) : build(std::forward<Args>(args)...){};
};

template <BuildableAndConsolidatable ResultType>
struct ConsolidateJob
{
    ResultType::Consolidator consolidator;

    ResultType consolidate();

    // template <typename... Args>
    // ConsolidateJob() = default; 
};

using ThreadPoolTask = std::function<void()>;

class ThreadPool
{
  private:
    // Pair so we can name the threads for debugging.
    std::vector<std::pair<std::thread, std::string>> m_threadsList;
    const size_t                                     m_numThreads;

  public:
    const size_t numThreads()
    {
        return m_numThreads;
    }

    void submitTask(ThreadPoolTask task);

    void joinTasks();

    ThreadPool(size_t numberOfThreads) : m_numThreads(numberOfThreads)
    {
    }
};

#endif // threadpool_hpp_INCLUDED
