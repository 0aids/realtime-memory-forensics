#include "test_helpers.hpp"
#include "logger.hpp"
#include <cstdlib>
#include <csignal>
#include <chrono>
#include <exception>
#include <memory>
#include <optional>
#include <sched.h>
#include <stdexcept>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <list>
#include <thread>
#include <algorithm>
#include <utility>
#include "abbreviations.hpp"

#pragma GCC push_options
#pragma GCC optimize("O0")

namespace rmf::test
{
    using namespace rmf::abv;
    using namespace std::chrono_literals;

    void testComponent::setup()
    {
        m_nextScheduledTime = std::chrono::steady_clock::now() + 1s;
    }

    void testComponent::execute()
    {
        // Do nothing
    }

    timepoint testComponent::reschedule()
    {
        m_nextScheduledTime = std::chrono::steady_clock::now() + 1s;
        return m_nextScheduledTime;
    }

    timepoint testComponent::getCurrentSchedule()
    {
        return m_nextScheduledTime;
    }

    void staticStringTestComponent::setup()
    {
        m_nextScheduledTime = std::chrono::steady_clock::now() + 5s;
    }

    void      staticStringTestComponent::execute() {}

    timepoint staticStringTestComponent::reschedule()
    {
        m_nextScheduledTime = std::chrono::steady_clock::now() + 5s;
        return m_nextScheduledTime;
    }

    timepoint staticStringTestComponent::getCurrentSchedule()
    {
        return m_nextScheduledTime;
    }

    incrementingIntComponent::incrementingIntComponent(
        int32_t initialValue, int32_t incrementAmount) :
        m_value(initialValue), m_incrementAmount(incrementAmount)
    {
    }

    void incrementingIntComponent::setup()
    {
        m_nextScheduledTime = std::chrono::steady_clock::now() + 1s;
    }

    void incrementingIntComponent::execute()
    {
        m_value += m_incrementAmount;
    }

    timepoint incrementingIntComponent::reschedule()
    {
        m_nextScheduledTime = std::chrono::steady_clock::now() + 1s;
        return m_nextScheduledTime;
    }

    timepoint incrementingIntComponent::getCurrentSchedule()
    {
        return m_nextScheduledTime;
    }

    int32_t incrementingIntComponent::getValue() const
    {
        return m_value;
    }

    staticValueComponent::staticValueComponent(int32_t int32value,
                                               int64_t int64value,
                                               float   float32value,
                                               double  float64value) :
        m_staticInt(int32value), m_staticLong(int64value),
        m_staticFloat(float32value), m_staticDouble(float64value)
    {
    }

    void staticValueComponent::setup()
    {
        m_nextScheduledTime = std::chrono::steady_clock::now() + 60s;
    }

    void      staticValueComponent::execute() {}

    timepoint staticValueComponent::reschedule()
    {
        m_nextScheduledTime = std::chrono::steady_clock::now() + 60s;
        return m_nextScheduledTime;
    }

    timepoint staticValueComponent::getCurrentSchedule()
    {
        return m_nextScheduledTime;
    }

    int32_t staticValueComponent::getStaticInt() const
    {
        return m_staticInt;
    }

    int64_t staticValueComponent::getStaticLong() const
    {
        return m_staticLong;
    }

    float staticValueComponent::getStaticFloat() const
    {
        return m_staticFloat;
    }

    double staticValueComponent::getStaticDouble() const
    {
        return m_staticDouble;
    }

    // > so that we get a min priority queue
    bool
    testComponentComparator::operator()(const sptr<testComponent> lhs,
                                        const sptr<testComponent> rhs)
    {
        return lhs->getCurrentSchedule() > rhs->getCurrentSchedule();
    }

    pid_t testProcess::run()
    {
        if (m_pid)
        {
            rmf_Log(rmf_Error, "Already running a test process!");
            return m_pid;
        }
        m_pid = fork();
        if (m_pid < 0)
            throw std::runtime_error("Failed to fork test process!");
        else if (m_pid > 0)
            return m_pid;

        // Set it so we die on parent process death.
        if (prctl(PR_SET_PDEATHSIG, SIGKILL) == -1)
        {
            perror("prctl failed");
            exit(EXIT_FAILURE);
        }

        // initialise stuff
        for (auto& component : m_componentHolder)
        {
            component->setup();
            component->reschedule();
            m_componentQueue.push(component);
        }

        while (true)
        {
            if (m_componentQueue.empty())
            {
                std::this_thread::sleep_for(1s);
                continue;
            }
            sptr<testComponent> component = m_componentQueue.top();
            m_componentQueue.pop();
            std::this_thread::sleep_until(
                component->getCurrentSchedule());
            component->execute();
            component->reschedule();
            m_componentQueue.push(component);
        }
    }

    void testProcess::stop()
    {
        rmf_Log(rmf_Verbose, "stopping test process!");
        kill(m_pid, SIGKILL);
        m_pid = 0;
    }

    testProcess::~testProcess()
    {
        if (m_pid)
            stop();
    }
} // namespace rmf::test
#pragma GCC pop_options
