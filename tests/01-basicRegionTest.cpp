// Testing retrieving region data.
#include "memory_map.hpp"
#include "memory_region.hpp"
#include <iostream>
using namespace std;
int main() {
  //
  pid_t pid = std::stoi(std::getenv("SAMPLE_PROCESS_PID"));
  MemoryAnalysis::MemoryMap a(pid);
  a.readMaps();

  MemoryAnalysis::BasicMemoryRegion region(&a["[stack]"]);

  if (region.snapshot(pid) == MemoryAnalysis::ERROR) {
    cerr << "Error encountered when snapshotting..." << endl;
    exit(1);
  }
  cout << "---- Stringified ----\n" << region.getLastSnapshot().toStr() << endl;
  MemoryAnalysis::BasicMemoryRegion region0(&a[0]);

  if (region0.snapshot(pid) == MemoryAnalysis::ERROR) {
    cerr << "Error encountered when snapshotting..." << endl;
    exit(1);
  }
  cout << "---- Stringified ----\n"
       << region0.getLastSnapshot().toStr() << endl;
};
