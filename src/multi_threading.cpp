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
            auto value = queue.tryDequeue();
            auto notification = queue.notifier.load(std::memory_order_acquire);
            if (value.has_value()) {
                value.value()();
                continue;
            }
            // Be very aggressive which checking for sleeping, otherwise
            // we might hang when destroying.
            if (!alive.load(std::memory_order_acquire)) {
                break;
            }
            rmf_Log(rmf_Debug, "Thread is waiting...");
            queue.notifier.wait(notification);
            rmf_Log(rmf_Debug, "Thread has woken!!!");
        }
        rmf_Log(rmf_Debug, "Thread is shutting down!");
    }
}
