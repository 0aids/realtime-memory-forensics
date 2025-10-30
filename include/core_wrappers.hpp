#ifndef core_wrappers_hpp_INCLUDED
#define core_wrappers_hpp_INCLUDED
#include <atomic>
#include <concepts>
#include <chrono>
#include <future>
#include <functional>
#include <vector>
#include <span>
#include <string>
#include <thread>

#include "MTQueue.hpp"
#include "core.hpp"
#include "logs.hpp"
#include "snapshots.hpp"
#include "maps.hpp"

/* 4 main separations of concerns:
 * Core function
     * This deals with processing any data. It takes in a massive struct of possible inputs.
         * The input struct would be a whole bunch of optional references to possible inputs required.
         * And they are all const.
     * This circumvents the need for having a whole bunch of problems with generics due to c++
     * lack of reflection, and allows core funcs to be ran seemlessly for given inputs.
 * Tasks
     * May contain metadata on order and debug, a future that holds the result type, and owning
       reference to the actual packaged task which would return the result type.
 * task creator/scheduler
     * A wrapper of this function would deal with mapping types of inputs
         (multiple small, one large, or just a singular)
       to core functions and returns a constructed lambda with the correct inputs captured.
     * Converts inputs and a corefunc, and order number of a task defined above.
 * task runner/mapper
     * A queue of tasks that need to be done (void lambdas containing the packaged tasks).
     * Threads will be watching the queue atomically until a void lambda has entered, in which it
     * will the run the lambda. This will actually allow us to run the tasks sequentially
     * without having to bunch up groups of them to be ran on a single thread manually beforehand like
     * before ;)
 * result reducer
     * Reduces a list of tasks into its results. Since the order is preserved within the tasks,
     there is no need for having metadata for order.
 * */

template <typename Callable>
using ReturnTypeOfT = typename decltype(std::function{
    std::declval<Callable>()})::result_type;

template <typename CoreFuncType>
struct Task
{
    std::future<ReturnTypeOfT<CoreFuncType>> result;
    std::packaged_task<ReturnTypeOfT<CoreFuncType>()>         packagedTask;
};

inline std::span<const char> spanFromRegionProperties(const MemoryRegionProperties &mrp, 
                                                    const MemorySnapshot &snap);

// How do I create multiple tasks?
template <typename CoreFuncType,
          typename... UniqueInputs>
auto createTask(CoreFuncType coreFunc, const CoreInputs& cInputs,
                UniqueInputs... uInputs)
{
    auto lambda = [=]() { return coreFunc(cInputs, uInputs...); };
    std::packaged_task ptask(lambda);

    // Idk why std::move is requried here, the operator= moves by default.
    return Task<CoreFuncType>{.result = ptask.get_future(), .packagedTask=std::move(ptask)};
}

template <typename CoreFuncType,
          typename... UniqueInputs>
auto createMultipleTasks(CoreFuncType coreFunc, const std::vector<CoreInputs> cInputsVec,
                         UniqueInputs... uInputs)
{
    std::vector<Task<CoreFuncType>> tasksVec;
    tasksVec.reserve(cInputsVec.size());

    for (const auto &cInputs : cInputsVec) 
    {
        tasksVec.push_back(createTask(coreFunc, cInputs, uInputs...));
    }

    return tasksVec;
}

template <typename T, typename FuncType>
concept ThreadPoolType = requires(T pool, Task<FuncType> task, 
                                    std::vector<Task<FuncType>> tasksVec)
{
    
    // Check for the templated 'submitTask' function
    {
        pool.submitTask(task)
    };

    // Check for the templated 'submitMultipleTasks' function
    {
        pool.submitMultipleTasks(tasksVec)
    };

    // Check for the non-templated 'awaitAllTasks' function
    { pool.awaitAllTasks() } -> std::same_as<void>; // Checks for existence and void return type
};

class QueuedThreadPool {
    using QTask = std::function<void()>;
    public:
        static constexpr auto TRYDURATION = std::chrono::milliseconds(10);
        const size_t pu_numThreads = 1;
        QueuedThreadPool(const size_t &numThreads) : pu_numThreads(numThreads) {
            m_threadsList.reserve(pu_numThreads);
            m_alive.store(true, std::memory_order_release);
            for (size_t i = 0; i < pu_numThreads; i++) {
                // The tasks queue is guaranteed to last until after all threads
                // are joined in the destructor.
                m_threadsList.emplace_back(
                    std::thread(threadFunctions, std::ref(m_alive), std::ref(m_tasksQueue)),
                    std::to_string(m_threadsList.size() + 1)
                );

                pthread_setname_np(m_threadsList.back().first.native_handle(),
                                   m_threadsList.back().second.c_str());
            }
        }
        ~QueuedThreadPool() {
            // Tell all threads to stop and then join all of them.
            m_alive.store(false, std::memory_order_release);
            for (auto &t : m_threadsList) {
                t.first.join();
            }
            return;
        }

    public:
        template <typename CoreFuncType>
        void submitTask(Task<CoreFuncType>& task) {
            m_tasksQueue.enqueue(
                [&packagedTask = task.packagedTask]()
                  {
                      packagedTask();
                  }
            );
        }

        template <typename CoreFuncType>
        inline void submitMultipleTasks(std::vector<Task<CoreFuncType>> &tasksVec)
        {
            for (auto &task : tasksVec) {
                submitTask(task);
            }
        }

        void awaitAllTasks() {
            using namespace std::chrono_literals;
            while (
                !m_tasksQueue.isEmpty()
            ) {
                std::this_thread::sleep_for(10ms);
            }
        }

    private:
        std::vector<std::pair<std::thread, std::string>> m_threadsList;
        std::atomic<bool> m_alive;
        SPMCQueue<QTask> m_tasksQueue;

        static void threadFunctions(std::atomic<bool> &alive, SPMCQueue<QTask> &q) {
            // Just check if we should still be alive, and then close if need be.
            while (alive.load(std::memory_order_acquire)) {
                auto func = q.tryDequeueFor(TRYDURATION);
                if (!func) continue;
                func.value()();
            }
        }

};

class TaskThreadPool
{
  public:
    const size_t pu_numThreads = 1;

    template <typename CoreFuncType>
    // Only takes the packaged task, but also takes ownership of it.
    void submitTask(Task<CoreFuncType>& task)
    {
        using namespace std::chrono;
        pr_threads.emplace_back(std::thread(
                                  [&packagedTask = task.packagedTask]() mutable
                                  {
                                      // For better print statements ig.
                                      std::this_thread::sleep_for(
                                          500us);
                                      packagedTask();
                                  }),
                              std::to_string(pr_threads.size() + 1));

        pthread_setname_np(pr_threads.back().first.native_handle(),
                           pr_threads.back().second.c_str());
    }
    template <typename CoreFuncType>
    void submitMultipleTasks(std::vector<Task<CoreFuncType>> &tasksVec)
    {
        for (auto &task : tasksVec) {
            submitTask(task);
        }
    }

    void awaitAllTasks() {
        if (pr_threads.size() == 0) {
            Log(Warning, "Attempted to join an empty thread pool");
            return;
        }
        for (auto &p : pr_threads) {
            p.first.join();
        }
        pr_threads.clear();
    }

    TaskThreadPool(size_t numThreads) : pu_numThreads(numThreads){}

  private:
    std::vector<std::pair<std::thread, std::string>> pr_threads;
};
// How do we consolidate them? For tasks that return a vector,
// we can join the vectors together. I think all tasks return a vector,
// so we can use a generic vector consolidator.

// Does not make any copies, and it moves all the items from the nested vectors
template <typename CoreFuncType>
ReturnTypeOfT<CoreFuncType> consolidateNestedTaskResults(std::vector<Task<CoreFuncType>> &tasks) {
    using OutputType = ReturnTypeOfT<CoreFuncType>;
    size_t numElements = 0;
    // Unconsolidated outputs -> unco
    std::vector<OutputType> uncoOutputs;
    uncoOutputs.reserve(tasks.size());

    for (auto &task : tasks) {
        uncoOutputs.push_back(task.result.get());
        numElements += uncoOutputs.back().size();
    }

    OutputType toReturn;
    toReturn.reserve(numElements);

    for (auto &vec : uncoOutputs) {
        std::move(
            vec.begin(), vec.end(), std::back_inserter(toReturn)
        );
    }

    // There better be nrvo
    return toReturn;
}


template <typename T>
std::vector<T> consolidateNestedVector(std::vector<std::vector<T>> &vector) {
    size_t numElements = 0;
    for (const auto &vec : vector) {
        numElements+=vec.size();
    }

    std::vector<T> toReturn;
    toReturn.reserve(numElements);

    for (auto &vec : vector) {
        std::move(
            vec.begin(), vec.end(), std::back_inserter(toReturn)
        );
    }

    // There better be nrvo
    return toReturn;
}

// Divides an mrp into multiple mrps, with differing relative region starts and sizes.
RegionPropertiesList divideSingleRegion(const MemoryRegionProperties& mrp, size_t quantity);

// A stupid form of reflection which would allow for
// perInput assignment.
struct MultipleCoreInputs {
    const std::optional<RegionPropertiesList> mrpVec = {};
    const std::optional<std::vector<MemorySnapshotSpan>> snap1Vec = {};
    const std::optional<std::vector<MemorySnapshotSpan>> snap2Vec = {};
};

std::vector<MemorySnapshotSpan> divideSingleSnapshot(const MemorySnapshot &snap, size_t quantity);

std::vector<CoreInputs> consolidateIntoCoreInput(
    const MultipleCoreInputs &c
);

// A stupid one for now, just for testing. It will just break the list up into spans.
std::vector<MemorySnapshotSpan> divideMultipleSnapshots(const std::vector<MemorySnapshot> &snapVec);

// Schedules them based on the mrps provided. Will break apart the mrps into smaller ones
// similarly with all the snaps.
std::vector<CoreInputs> scheduleMultipleCoreInputs(
    const MultipleCoreInputs &mci,
    const uintptr_t taskByteSizes
);

#endif // core_wrappers_hpp_INCLUDED
