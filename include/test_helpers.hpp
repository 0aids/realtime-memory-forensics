#ifndef test_helpers_hpp_INCLUDED
#define test_helpers_hpp_INCLUDED
#include <sched.h>

namespace rmf::test
{
    pid_t startTestFunction();
}

#define rmf_Assert(shouldBeTrue, FailureMessage)                     \
    if (!(shouldBeTrue))                                               \
    {                                                                \
        rmf_Log(rmf_Error, "Assertion failed: " << FailureMessage);  \
    }                                                                \
    else                                                             \
    {                                                                \
        rmf_Log(rmf_Debug, "Assertion succeeded.");                  \
    }

#endif // test_helpers_hpp_INCLUDED
