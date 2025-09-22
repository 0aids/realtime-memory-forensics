#include "memory_map.hpp"
#include "region_properties.hpp"
#include <cstdlib>
#include <iostream>
#include <sched.h>

using namespace std;
#define LOG(str) cout << str << endl;

int main() {
  pid_t pid = std::stoi(std::getenv("SAMPLE_PROCESS_PID"));
  MemoryAnalysis::MemoryMap a(pid);
  a.readMaps();
  LOG("Read the maps!");
  MemoryAnalysis::Properties::RegionPropertiesList regionProperties_l =
      a.getPropertiesList();

  for (auto &properties : regionProperties_l) {
    cout << &properties << endl;
  }

  return 0;
}
