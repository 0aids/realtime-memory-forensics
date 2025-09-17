#pragma once
#include <iostream>
#include <ostream>
#include <vector>

#define DEBUG(str) std::cout << "[ DEBUG ] " << str << std::endl;
#define ERROR(str) std::cerr << "[!ERROR!] " << str << std::endl;

template <typename T>
std::ostream &operator<<(std::ostream &os, std::vector<T> &value) {
  os << "[ ";
  for (T &val : value) {
    os << val << ", ";
  }
  os << "]";
  return os;
}
