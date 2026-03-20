#ifndef multi_threading_hpp_INCLUDED
#define multi_threading_hpp_INCLUDED
#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <pthread.h>
#include <semaphore>
#include <thread>
#include <ranges>
#include "logger.hpp"
#include "utils.hpp"
#include <future>
#include <tuple>
#include <type_traits>
#include <utility>

namespace rmf
{
    class TaskThreadPool_t;

    template <typename Return_t>
    class Task_t
    {
      private:
        struct Data
        {
            std::packaged_task<Return_t()> packagedTask;
            std::future<Return_t>          future;
        };
        // Handle body idiom, now trivially copyable. No more stupid reference going out of scope.
        std::shared_ptr<Data> d;

      public:
        inline std::future<Return_t>& getFuture()
        {
            return d->future;
        }
        inline std::packaged_task<Return_t()>& getPackagedTask()
        {
            return d->packagedTask;
        }
        explicit operator bool() const
        {
            return d->future.valid();
        }

        template <typename Func_t, typename... Args>
        Task_t(const Func_t func, Args&&... inputs)
        {
            // Copies, so if you want to be fast always use trivially copyable.
            // for example handle-body idiom: hidden shared pointers.
            // No more lifetime problems hopefully.
            using stdFuncType  = decltype(std::function{func});
            using FuncReturn_t = stdFuncType::result_type;
            static_assert(
                std::is_invocable_v<Func_t, Args...>,
                "Arguments do not match function signature");
            static_assert(std::is_same_v<FuncReturn_t, Return_t>,
                          "Inputted function and templated return "
                          "type are different!");
            d = std::make_shared<Data>(
                std::packaged_task<Return_t()>(
                    [argsTuple = std::forward_as_tuple(inputs...),
                     func]() { return std::apply(func, argsTuple); }),
                std::future<Return_t>{});
            d->future = d->packagedTask.get_future();
        }
    };

    class TaskThreadPool_t
    {
      private:
        utils::SPMCQueue<std::function<void()>> m_queue;
        std::vector<std::thread>                m_threads;
        std::vector<std::string>                m_threadNames;
        std::atomic<bool>                       m_alive;
        std::atomic<uint32_t>                   m_numRunning;

        static void
        threadFunction(const std::atomic<bool>&                 alive,
                       utils::SPMCQueue<std::function<void()>>& queue,
                       std::atomic<uint32_t>& numRunning);

      public:
        double getTasksPerSecond(std::chrono::milliseconds ms)
        {
            uint64_t initialIndex = m_queue.getConsumeIndex();
            std::this_thread::sleep_for(ms);
            uint64_t endIndex = m_queue.getConsumeIndex();
            if (endIndex < initialIndex)
                endIndex += m_queue.size;
            uint64_t numCompleted = endIndex - initialIndex;
            return numCompleted /
                std::chrono::duration<double>(ms).count();
        }
        // Tasks' data are held by a shared ptr, so they will not die, and are trivially copyable.
        template <typename T>
        void SubmitTask(Task_t<T> task)
        {
            while (!m_queue.enqueue([task]() mutable
                                    { task.getPackagedTask()(); }))
            {
                using namespace std::chrono_literals;
                rmf_Log(
                    rmf_Warning,
                    "Failed to enqueue task! Task Queue is Full!!!");
                std::this_thread::sleep_for(1s);
            }
        }

        template <typename T, typename func_t>
        void SubmitTask(Task_t<T> task, func_t yieldingCallback)
        {
            while (!m_queue.enqueue([task]() mutable
                                    { task.getPackagedTask()(); }))
            {
                using namespace std::chrono_literals;
                rmf_Log(
                    rmf_Warning,
                    "Failed to enqueue task! Task Queue is Full!!!");
                yieldingCallback();
            }
        }

        template <std::ranges::range TaskList>
        void SubmitMultipleTasks(TaskList& tasks)
        {
            for (auto& task : tasks)
            {
                this->SubmitTask(task);
            }
        }

        void AwaitTasks()
        {
            using namespace std::chrono_literals;
            while (!m_queue.empty() ||
                   m_numRunning.load(std::memory_order_acquire) > 0)
            {
                std::this_thread::sleep_for(1ms);
            }
        }

        // For the python constructor.
        static TaskThreadPool_t makeThreadPool(size_t numThreads)
        {
            return TaskThreadPool_t(numThreads);
        }

        TaskThreadPool_t(size_t numThreads) :
            m_queue(rmf::utils::d_defaultQueueSize), m_alive(true),
            m_numRunning(0)
        {
            for (size_t i = 0; i < numThreads; i++)
            {
                m_threads.emplace_back(
                    threadFunction, std::ref(m_alive),
                    std::ref(m_queue), std::ref(m_numRunning));
                m_threadNames.emplace_back(std::to_string(i + 1));

                pthread_setname_np(m_threads.back().native_handle(),
                                   m_threadNames.back().c_str());
            }
        }

        ~TaskThreadPool_t()
        {
            m_alive.store(false, std::memory_order_release);
            for (auto& thread : m_threads)
            {
                thread.join();
            }
        }
    };
}

#endif // multi_threading_hpp_INCLUDED
