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
#pragma GCC push_options
#pragma GCC optimize("O0")
namespace rmf::test
{
    using namespace rmf::abv;
    using namespace std::chrono;
    using timepoint =
        std::chrono::time_point<std::chrono::steady_clock>;
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
            "lorem ipsumlorem ipsumlorem ipsumlorem ipsumlorem "
            "ipsumlorem ipsumlorem ipsumlorem ipsumlorem ipsumlorem "
            "ipsumlorem ipsumlorem ipsumlorem ipsumlorem ipsumlorem "
            "ipsumlorem ipsumlorem ipsumlorem ipsumlorem ipsumlorem "
            "ipsumlorem ipsumlorem ipsumlorem ipsumlorem ipsumlorem "
            "ipsumlorem ipsumlorem ipsumlorem ipsumlorem ipsumlorem "
            "ipsumlorem ipsumlorem ipsumlorem ipsumlorem ipsumlorem "
            "ipsumlorem ipsumlorem ipsumlorem ipsumlorem ipsumlorem "
            "ipsumlorem ipsumlorem ipsumlorem ipsumlorem ipsumlorem "
            "ipsumlorem ipsumlorem ipsumlorem ipsumlorem ipsumlorem "
            "ipsumlorem ipsumlorem ipsumlorem ipsumlorem ipsumlorem "
            "ipsumlorem ipsumlorem ipsumlorem ipsumlorem ipsumlorem "
            "ipsumlorem ipsumlorem ipsumlorem ipsumlorem ipsumlorem "
            "ipsumlorem ipsumlorem ipsumlorem ipsumlorem ipsumlorem "
            "ipsumlorem ipsumlorem ipsumlorem ipsumlorem ipsumlorem "
            "ipsumlorem ipsumlorem ipsumlorem ipsumlorem ipsumlorem "
            "ipsumlorem ipsumlorem ipsumlorem ipsumlorem ipsumlorem "
            "ipsumlorem ipsumlorem ipsumlorem ipsumlorem ipsumlorem "
            "ipsumlorem ipsumlorem ipsumlorem ipsumlorem ipsumlorem "
            "ipsumlorem ipsumlorem ipsumlorem ipsumlorem ipsumlorem "
            "ipsum";

      public:
        void      setup() override;
        void      execute() override;
        timepoint reschedule() override;
        timepoint getCurrentSchedule() override;
    };

    class incrementingIntComponent : public testComponent
    {
      private:
        int32_t m_value           = 0;
        int32_t m_incrementAmount = 1;

      public:
        explicit incrementingIntComponent(
            int32_t initialValue = 0, int32_t incrementAmount = 1);
        void      setup() override;
        void      execute() override;
        timepoint reschedule() override;
        timepoint getCurrentSchedule() override;
        int32_t   getValue() const;
    };

    class staticValueComponent : public testComponent
    {
      private:
        int32_t m_staticInt;
        int64_t m_staticLong;
        float   m_staticFloat;
        double  m_staticDouble;

      public:
        explicit staticValueComponent(
            int32_t int32value   = 0x99998888,
            int64_t int64value   = 1234567890123,
            float   float32value = 3.141519f,
            double  float64value = 2.718281828);
        void      setup() override;
        void      execute() override;
        timepoint reschedule() override;
        timepoint getCurrentSchedule() override;
        int32_t   getStaticInt() const;
        int64_t   getStaticLong() const;
        float     getStaticFloat() const;
        double    getStaticDouble() const;
    };

    class staticLargeEmptyComponent : public testComponent
    {
      private:
        void* m_buffer = nullptr;
        size_t m_bufferSize = 0;

      public:
        // Defaults to size of 2gb to search
        staticLargeEmptyComponent(size_t size = 2 << 30);
        ~staticLargeEmptyComponent();
        void setup() override;
        using testComponent::execute;
        using testComponent::reschedule;
        using testComponent::getCurrentSchedule;
    };

    template <typename T>
    class SListComponent : public testComponent
    {
      private:
        struct Node
        {
            T     data;
            Node* next;
        };
        std::vector<Node*> m_nodes;
        Node*              m_head;
        std::vector<T>     m_values;

      public:
        using timepoint =
            std::chrono::time_point<std::chrono::steady_clock>;

        explicit SListComponent(std::vector<T> values);
        ~SListComponent();

        void      setup() override;
        void      execute() override;
        timepoint reschedule() override;
        timepoint getCurrentSchedule() override;

        Node*     getHead();
        Node*     getNodeAt(size_t index);
        size_t    size() const;
        uintptr_t getHeadAddress() const;
        uintptr_t getNodeAddress(size_t index) const;
    };

    class testProcess
    {
      private:
        std::priority_queue<sptr<testComponent>,
                            std::vector<sptr<testComponent>>,
                            testComponentComparator>
                                         m_componentQueue  = {};
        std::vector<sptr<testComponent>> m_componentHolder = {};
        pid_t                            m_pid             = 0;

      public:
        // Will fork itself, setup all components, generate the priority queue
        // and then run.
        testProcess()                             = default;
        testProcess operator=(const testProcess&) = delete;
        testProcess(const testProcess&)           = delete;
        testProcess(testProcess&&)                = default;
        ~testProcess();
        pid_t run();

        template <typename T, typename... Args>
        testProcess& build(Args&&... args)
        {
            static_assert(std::is_base_of<testComponent, T>(),
                          "T must derive from testComponent");

            m_componentHolder.push_back(
                std::make_shared<T>(std::forward<Args>(args)...));
            return *this;
        }
        void stop();
    };
} // namespace rmf::test

#pragma GCC pop_options

#endif // test_helpers_hpp_INCLUDED
