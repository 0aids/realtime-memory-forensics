#ifndef tests_hpp_INCLUDED
#define tests_hpp_INCLUDED
#include "logs.hpp"
#include <unistd.h>

// Returns the PID of the sample process.

pid_t runSampleProcess();

// Check if the ptrace scope is 0. returns true if true.
// Not needed for the tests, but otherwise might need.
bool checkPtraceScope();

#define assert(shouldBeTrue, message)                                \
    if (!(shouldBeTrue)) {                                           \
        Log(Error, message);                                         \
        exit(EXIT_FAILURE);                                          \
    } else {                                                         \
        Log(Debug, "Assertion succeeded")                            \
    }

pid_t runChangingMapProcess();

#endif // tests_hpp_INCLUDED
