#include "multi_threading.hpp"
#include "logger.hpp"
#include "types.hpp"
#include "utils.hpp"
#include <atomic>
#include <type_traits>
namespace rmf
{
    void TaskThreadPool_t::threadFunction(
        const std::atomic<bool>&                 alive,
        utils::SPMCQueue<std::function<void()>>& queue,
        std::atomic<uint32_t>&                   numRunning

    )
    {
        using namespace std::chrono_literals;
        while (alive.load(std::memory_order_acquire))
        {
            auto value = queue.tryDequeueFor(10ms);
            if (value.has_value())
            {
                numRunning.fetch_add(1, std::memory_order_release);
                rmf_Log(rmf_Debug, "Thread has found a task!");
                value.value()();
                numRunning.fetch_sub(1, std::memory_order_release);
                continue;
            }
        }
        rmf_Log(rmf_Debug, "Thread is shutting down!");
    }
}
