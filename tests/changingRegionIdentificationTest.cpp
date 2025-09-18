#include "../include/obtainMemory.hpp"
#include "../include/testing.hpp"
#include <cstdlib>
#include <sched.h>

using namespace std;

// A test for identifying reginos with changing parts of memory.
int main() {
  pid_t pid = getPidFromEnv();

  ProgramMemory pm(pid);
  pm.setup();

  return 0;
}
