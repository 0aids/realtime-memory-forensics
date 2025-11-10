#ifndef tests_hpp_INCLUDED
#define tests_hpp_INCLUDED
#include "utils/logs.hpp"
#include "data/refreshable_snapshots.hpp"
#include <unistd.h>

#define rmf_assert(shouldBeTrue, message)                                \
    if (!(shouldBeTrue)) {                                           \
        rmf_Log(rmf_Error, message);                                         \
        exit(EXIT_FAILURE);                                          \
    } else {                                                         \
        rmf_Log(rmf_Debug, "Assertion succeeded")                            \
    }
// Returns the PID of the sample process.
namespace rmf::tests {
    using namespace rmf::data;

pid_t runSampleProcess();

// Check if the ptrace scope is 0. returns true if true.
// Not needed for the tests, but otherwise might need.
bool checkPtraceScope();


pid_t runChangingMapProcess();

std::pair<std::shared_ptr<RefreshableSnapshot>,
          std::shared_ptr<RefreshableSnapshot>>
getSampleRefreshableSnapshots(pid_t sampleProcessPID);
}

#endif // tests_hpp_INCLUDED
