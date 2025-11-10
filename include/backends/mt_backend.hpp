#ifndef core_wrappers_hpp_INCLUDED
#define core_wrappers_hpp_INCLUDED
#include <atomic>
#include <chrono>
#include <future>
#include <functional>
#include <vector>
#include <span>
#include <string>
#include <thread>

#include "utils/MTQueue.hpp"
#include "core.hpp"
#include "data/snapshots.hpp"
#include "data/maps.hpp"

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

namespace rmf::backends::mt {
using namespace rmf::backends::core;
using namespace rmf;

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
auto createTask(CoreFuncType coreFunc, const CoreInputs cInputs,
                UniqueInputs... uInputs)
{
    // task number needs to be captured otherwise the logs don't show for some reason.
    auto lambda = [=]() { 
        // if (taskNumber % 1000 == 0)
        //     Log(Warning, "Task number: " << taskNumber);
        return coreFunc(cInputs, uInputs...); };
    std::packaged_task ptask(lambda);

    // Idk why std::move is requried here, the operator= moves by default.
    auto t = Task<CoreFuncType>{.result = {}, .packagedTask=std::move(ptask)};
    t.result = t.packagedTask.get_future();
    return t;
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

                // For debugging purposes.
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

        template <std::ranges::range TaskList>
        inline void submitMultipleTasks(TaskList &tasksVec)
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
        utils::SPMCQueue<QTask> m_tasksQueue;

        static void threadFunctions(std::atomic<bool> &alive, utils::SPMCQueue<QTask> &q) {
            // Just check if we should still be alive, and then close if need be.
            while (alive.load(std::memory_order_acquire)) {
                auto func = q.tryDequeueFor(TRYDURATION);
                if (!func) continue;
                func.value()();
            }
        }
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

// Converts a list of snapshots into its equivalent list of spans.
std::vector<MemorySnapshotSpan> makeSnapshotSpans(const std::vector<MemorySnapshot> &snapVec);

std::vector<MemorySnapshot> convertTasksIntoSnapshots(std::vector<Task<MemorySnapshot (*)(const CoreInputs &)>> &tasks);
};
#endif // core_wrappers_hpp_INCLUDED
