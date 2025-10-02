#pragma once
#include <unistd.h>
#include <string>

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
