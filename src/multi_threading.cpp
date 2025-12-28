#include "multi_threading.hpp"
#include "utils.hpp"
#include <atomic>
#include <type_traits>
namespace rmf
{
    void TaskThreadPool_t::threadFunction(
        const std::atomic<bool>& alive,
        utils::SPMCQueueNonOwning<std::function<void()>, d_defaultQueueSize>&
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
            queue.notifier.wait(queue.notifierWakeValue);
        }
    }
}
