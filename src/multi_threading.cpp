#include "multi_threading.hpp"
#include "logger.hpp"
#include "types.hpp"
#include "utils.hpp"
#include <atomic>
#include <type_traits>
namespace rmf
{
    void TaskThreadPool_t::threadFunction(
        const std::atomic<bool>& alive,
        utils::SPMCQueue<std::function<void()>>&
            queue
    )
    {
        using namespace std::chrono_literals;
        while (alive.load(std::memory_order_acquire))
        {
            auto value = queue.tryDequeueFor(10ms);
            if (value.has_value()) {
                rmf_Log(rmf_Debug, "Thread has found a task!");
                value.value()();
                continue;
            }
        }
        rmf_Log(rmf_Debug, "Thread is shutting down!");
    }
}
