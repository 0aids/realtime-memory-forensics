// Random ass idea: A resizable, circular array, which allows popping from both ends quickly?
#ifndef threadpool_hpp_INCLUDED
#define threadpool_hpp_INCLUDED
#include <chrono>
#include "logs.hpp"
#include <future>
#include <thread>
#include <vector>
#include <utility>
#include <unistd.h>
#include <functional>

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


    template <typename FunctionType, typename OutputType>
    std::future<OutputType> submitTask(std::packaged_task<FunctionType> task) 
    {
        using namespace std::chrono;
        // NOTE: Switch a queue based systems with less threads.
        // if (m_threadsList.size() >= m_numThreads)
        // {
        //     Log(Warning, "Attempted to use more threads than allocated in constructor");
        //     return;
        // }
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

        return task.get_future();
    }

    void joinTasks();

    ThreadPool(size_t numberOfThreads) : m_numThreads(numberOfThreads)
    {
    }
};

#endif // threadpool_hpp_INCLUDED
