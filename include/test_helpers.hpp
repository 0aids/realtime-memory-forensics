#ifndef test_helpers_hpp_INCLUDED
#define test_helpers_hpp_INCLUDED
#include <sched.h>
#include <functional>

namespace rmf::test
{
    void  unchangingProcess1();
    void  changingProcess1();
    pid_t forkTestFunction(std::function<void()> function);
}

#define rmf_Assert(shouldBeTrue, FailureMessage)                     \
    if (!(shouldBeTrue))                                             \
    {                                                                \
        rmf_Log(rmf_Error, "Assertion failed: " << FailureMessage);  \
        exit(EXIT_FAILURE);                                          \
    }                                                                \
    else                                                             \
    {                                                                \
        rmf_Log(rmf_Debug, "Assertion succeeded.");                  \
    }

#endif // test_helpers_hpp_INCLUDED
