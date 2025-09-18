#include "../include/obtainMemory.hpp"
#include "../include/testing.hpp"
#include <cstdlib>
#include <sched.h>

using namespace std;

int main() {
  pid_t pid = getPidFromEnv();
  DEBUG("This is the searching memory test");
  DEBUG("pid: " << pid);

  ProgramMemory pm(pid);
  pm.setup();
  DEBUG("Setup complete");
  pm.printMemoryRegionListNames();
  DEBUG("Printing regions");
  pm.searchFirst("This is some extra shit");
  pm.searchFirst("small");
  pm.searchFirst("HAIL");
  pm.searchFirst(
      " Lorem ipsum dolor sit amet, consectetur adipiscing elit. Mauris "
      "commodo, felis non semper fermentum, lorem velit dictum risus, id "
      "rutrum ipsum nisi eget ex. Curabitur ullamcorper consectetur venenatis. "
      "Vestibulum ultrices iaculis arcu quis suscipit. Duis tincidunt ");
  // Intentionally fail.
  pm.searchFirst("aaaaaaadkfjeqk");

  return 0;
}
