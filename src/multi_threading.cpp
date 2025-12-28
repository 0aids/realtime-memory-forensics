#include "multi_threading.hpp"
#include "logger.hpp"
#include "utils.hpp"
#include <atomic>
#include <type_traits>
namespace rmf
{
    void TaskThreadPool_t::threadFunction(
        const std::atomic<bool>& alive,
        utils::SPMCQueueNonOwning<std::function<void()>>&
            queue
    )
    {
        using namespace std::chrono_literals;
        while (alive.load(std::memory_order_acquire))
        {
            auto value = queue.tryDequeue();
            if (value.has_value()) {
                value.value()();
                continue;
            }
            rmf_Log(rmf_Debug, "Thread is waiting...");
            queue.notifier.wait(queue.notifier.load(std::memory_order_acquire));
            rmf_Log(rmf_Debug, "Thread has woken!!!");
        }
        rmf_Log(rmf_Debug, "Thread is shutting down!");
    }
}
