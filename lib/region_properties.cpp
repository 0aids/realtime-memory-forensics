#include "region_properties.hpp"
#include <sstream>
#include <string>
namespace MemoryAnalysis::Properties {

std::string maskToString(PermissionsMask perms) {
#define R 0
#define W 1
#define X 2
#define P 3
  std::string mask = "....";
  if ((perms & READ) != 0) {
    mask[R] = 'r';
  }
  if ((perms & WRITE) != 0) {
    mask[W] = 'w';
  }
  if ((perms & EXECUTE) != 0) {
    mask[X] = 'x';
  }
  if ((perms & PRIVATE) != 0) {
    mask[P] = 'p';
  }
#undef R
#undef W
#undef X
#undef P

  return mask;
}
PermissionsMask charsToMask(char perms[4]) {
#define R 0
#define W 1
#define X 2
#define P 3
  PermissionsMask mask = 0;
  if (perms[R] == 'r') {
    mask |= READ;
  }
  if (perms[W] == 'w') {
    mask |= WRITE;
  }
  if (perms[X] == 'x') {
    mask |= EXECUTE;
  }
  if (perms[P] == 'p') {
    mask |= PRIVATE;
  }
#undef R
#undef W
#undef X
#undef P

  return mask;
}

BasicRegionProperties::BasicRegionProperties(std::string name, uintptr_t start,
                                             size_t size, PermissionsMask perms)
    : m_name(name), m_start(start), m_size(size), m_perms(perms) {}

std::string BasicRegionProperties::toStr() {
  std::stringstream str;
  str << "Region Name: " << m_name << "\n";
  str << "Start Address: " << std::showbase << std::hex << m_start << "\n";
  str << "Region Size: " << std::showbase << std::hex << m_size << "\n";
  str << "Perms: " << Properties::maskToString(m_perms) << "\n";
  return str.str();
}
const std::string BasicRegionProperties::toConstStr() const {
  std::stringstream str;
  str << "Region Name: " << m_name << "\n";
  str << "Start Address: " << std::showbase << std::hex << m_start << "\n";
  str << "Region Size: " << std::showbase << std::hex << m_size << "\n";
  str << "Perms: " << Properties::maskToString(m_perms) << "\n";
  return str.str();
}

MemoryRegionProperties::MemoryRegionProperties(std::string name,
                                               uintptr_t start, size_t size,
                                               PermissionsMask perms)
    : BasicRegionProperties(name, start, size, perms) {}

std::ostream &operator<<(std::ostream &os, BasicRegionProperties *properties) {
  os << properties->toStr();
  return os;
}
std::ostream &operator<<(std::ostream &os,
                         const BasicRegionProperties *properties) {
  os << properties->toConstStr();
  return os;
}

}; // namespace MemoryAnalysis::Properties
