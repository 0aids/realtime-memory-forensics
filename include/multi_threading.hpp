#ifndef multi_threading_hpp_INCLUDED
#define multi_threading_hpp_INCLUDED
#include <atomic>
#include <functional>
#include <pthread.h>
#include <thread>
#include <ranges>
#include "logger.hpp"
#include "utils.hpp"
#include <future>
#include <tuple>
#include <utility>

namespace rmf
{
    constexpr size_t d_defaultQueueSize = 1 << 18;
    class TaskThreadPool_t;

    template <typename Func_t>
    class Task_t
    {
      public:
        using Return_t = Func_t::result_type;

        std::future<Return_t>      future;
        std::packaged_task<Return_t()> packagedTask;

        explicit                   operator bool() const
        {
            return future.valid();
        }

        template <typename... Args>
        Task_t(const Func_t func, Args&&... inputs)
        {
            auto argsTuple = std::forward_as_tuple(std::forward<Args>(inputs)...);
            packagedTask = std::packaged_task<Return_t()>(
                [=]() { return std::apply(func, argsTuple); });
            future = packagedTask.get_future();
        }
    };

    // TODO: Implement a turn-key system using std::move to ensure lapping doesn't occur.
    // Means modifying SPMCQueueNonOwning to have std::pair<atomic<uint32_t>, T>
    // Add a custom dequeue that checks the turn-key.
    class TaskThreadPool_t
    {
      private:
        utils::SPMCQueueNonOwning<std::function<void()>
                                >
                                 m_queue;
        std::vector<std::thread> m_threads;
        std::vector<std::string> m_threadNames;
        std::atomic<bool>        m_alive;

        static void                     threadFunction(
                                const std::atomic<bool>&                       alive,
                                utils::SPMCQueueNonOwning<std::function<void()>>& queue
                                );

      public:
        // The threadpool does not own tasks, as they provide a reference
        // to which we can complete data to.
        // The task MUST NOT DIE until it has been completed.
        template <typename T>
        void SubmitTask(Task_t<T>& task)
        {
            while (!m_queue.enqueue([&task]() { task.packagedTask(); }))
            {
                rmf_Log(rmf_Warning, "Failed to enqueue task! Task Queue is Full!!!");
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
            while (!m_queue.empty())
            {
                std::this_thread::sleep_for(10ms);
            }
        }

        TaskThreadPool_t(size_t numThreads) : m_queue(d_defaultQueueSize), m_alive(true)
        {
            for (size_t i = 0; i < numThreads; i++) {
                m_threads.emplace_back(threadFunction, std::ref(m_alive), std::ref(m_queue));
                m_threadNames.emplace_back(std::to_string(i + 1));

                pthread_setname_np(
                    m_threads.back().native_handle(), m_threadNames.back().c_str());
            }
        }

        ~TaskThreadPool_t(){
            // The only problem with lock-free is sometimes someone doesn't want to join.
            // This is why there is a notifier spam here.
            m_alive.store(false, std::memory_order_release);
            m_queue.notifier.fetch_add(1, std::memory_order_release);
            m_queue.notifier.notify_all();
            for (auto &thread:m_threads) {
                m_queue.notifier.fetch_add(1, std::memory_order_release);
                m_queue.notifier.notify_all();
                thread.join();
            }
        }
    };
}

#endif // multi_threading_hpp_INCLUDED
