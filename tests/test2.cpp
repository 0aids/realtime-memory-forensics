#include "../include/obtainMemory.hpp"
#include "../include/testing.hpp"
#include <cstdlib>
#include <sched.h>

using namespace std;

int main() {
  pid_t pid = getPidFromEnv();

  ProgramMemory pm(pid);
  pm.setup();
  pm.printMemoryRegionListNames();
  pm.searchFirst("This is some extra shit");
  pm.searchFirst("small");
  pm.searchFirst(
      " Lorem ipsum dolor sit amet, consectetur adipiscing elit. Mauris "
      "commodo, felis non semper fermentum, lorem velit dictum risus, id "
      "rutrum ipsum nisi eget ex. Curabitur ullamcorper consectetur venenatis. "
      "Vestibulum ultrices iaculis arcu quis suscipit. Duis tincidunt ");

  return 0;
}
