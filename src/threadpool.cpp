#include "threadpool.hpp"
#include <pthread.h>
#include "logs.hpp"

// void ThreadPool::submitTask(ThreadPoolTask task)
// {
//     // The fucking print statements go haywire because the thread
//     // starts before everything is pushed back...
//     // Also the string is needed otherwise we don't have the correct lifetime.
//     using namespace std::chrono;
//     if (m_threadsList.size() >= m_numThreads)
//     {
//         Log(Warning, "Attempted to use more threads than allocated in constructor");
//         return;
//     }
//     m_threadsList.push_back(
//         {std::thread(
//             [task](){
//                 // For better print statements ig.
//                 std::this_thread::sleep_for(500us);
//                 task();
//             }
//         ),
//          std::to_string(m_threadsList.size() + 1)});

//     pthread_setname_np(m_threadsList.back().first.native_handle(),
//                        m_threadsList.back().second.c_str());
// }

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
