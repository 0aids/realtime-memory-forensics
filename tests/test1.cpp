#include "../include/obtainMemory.hpp"
#include <cstdlib>
#include <iostream>
#include <sched.h>

using namespace std;

int main() {
  pid_t pid = std::stoi(std::getenv("SAMPLE_PROCESS_PID"));
  MemoryMap a(pid);
  a.readMaps();
  auto region = a.getRegionProperties();
  cout << region << endl;

  return 0;
}
