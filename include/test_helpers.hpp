#ifndef test_helpers_hpp_INCLUDED
#define test_helpers_hpp_INCLUDED

#include <chrono>
#include "types.hpp"
#include <concepts>
#include <functional>
#include <atomic>
#include <memory>
#include <queue>
#include <sched.h>
#include <string>
#include <thread>
#include <vector>
#include <cstdint>
#include <variant>
#include <optional>
#include "abbreviations.hpp"

/*
 * Need to be able to construct tests. Tests processes should be
 * event based, making use of a pq to schedule events and process
 * them at relevant times.
 *
 * The ideal scenario
 * */
namespace rmf::test
{
    using namespace rmf::abv;
    using namespace std::chrono;
    using timepoint = std::chrono::time_point<std::chrono::steady_clock>;
    class testComponent
    {
      protected:
        timepoint m_nextScheduledTime;

      public:
        testComponent() = default;
        virtual void setup();
        virtual void execute();
        // By default will reschedule 1s from now
        virtual timepoint reschedule();
        virtual timepoint getCurrentSchedule();
    };
    struct testComponentComparator
    {
        bool operator()(const sptr<testComponent> lhs,
                        const sptr<testComponent> rhs);
    };

    class staticStringTestComponent : public testComponent
    {
      private:
        std::string smallString = "I am a small string";
        std::string bigString =
            "lorem ipsum"; // Multiplied by 100 during startup.
      public:
        void        setup() override;
        void        execute() override;
        timepoint reschedule() override;
        timepoint getCurrentSchedule() override;
    };

    class testProcess
    {
      private:
        std::priority_queue<sptr<testComponent>,
                            std::vector<sptr<testComponent>>,
                            testComponentComparator>
                                         m_componentQueue = {};
        std::vector<sptr<testComponent>> m_componentHolder = {};
        pid_t                            m_pid = 0;

      public:
        // Will fork itself, setup all components, generate the priority queue
        // and then run.
        testProcess() = default;
        testProcess operator=(const testProcess&) = delete;
        testProcess(const testProcess&) = delete;
        testProcess(testProcess&&) = default;
        ~testProcess();
        pid_t       run();

        template <typename T, typename... Args>
        testProcess& build(Args&&... args)
        {
            static_assert(std::is_base_of<testComponent, T>(), "T must derive from testComponent");

            m_componentHolder.push_back(std::make_shared<T>(std::forward<Args>(args)...));
            return *this;
        }
        void        stop();
    };
} // namespace rmf::test

// Legacy assertion macro (kept for backward compatibility)
#define rmf_Assert(shouldBeTrue, FailureMessage)                     \
    if (!(shouldBeTrue))                                             \
    {                                                                \
        rmf_Log(rmf_Error, "Assertion failed: " << FailureMessage);  \
        exit(EXIT_FAILURE);                                          \
    }                                                                \
    else                                                             \
    {                                                                \
        rmf_Log(rmf_Info, "Assertion succeeded.");                   \
    }

#endif // test_helpers_hpp_INCLUDED
