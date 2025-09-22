#include "memory_map.hpp"
#include "memory_region.hpp"
#include "region_properties.hpp"
#include <chrono>
#include <iostream>
#include <thread>

using namespace std;
using namespace MemoryAnalysis;
int main() {
  //
  pid_t pid = std::stoi(std::getenv("SAMPLE_PROCESS_PID"));
  MemoryAnalysis::MemoryMap a(pid);
  a.readMaps();

  auto properties_l = a.getPropertiesList();

  for (auto &properties : properties_l) {
    if ((properties.m_perms & Properties::WRITE) == 0) {
      continue;
    }
    BasicMemoryRegion region(&properties);
    region.snapshot(pid);
    this_thread::sleep_for(chrono::milliseconds(100));
    region.snapshot(pid);
    auto changes = region.getChanged(8);
    if (!changes.empty()) {
      cout << "Found moving region at memory address: " << std::showbase
           << std::hex << changes.back() << endl;
      cout << "In region: \n";
      cout << &properties << endl;
      cout << "Num changes: " << changes.size() << endl;
      break;
    }
  }

  // Unchanging test
  BasicMemoryRegion region(&a["[stack]"]);
  region.snapshot(pid);
  this_thread::sleep_for(chrono::milliseconds(100));
  region.snapshot(pid);
  auto changes = region.getUnchanged(8);
  if (!changes.empty()) {
    cout << "Found unmoving region at memory address: " << std::showbase
         << std::hex << changes.back() << endl;
    cout << "In region: \n";
    auto val = &a["[stack]"];
    cout << &a["[stack]"];
    cout << "Num unchanges: " << changes.size() << endl;
  }
}
