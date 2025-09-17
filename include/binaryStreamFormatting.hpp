#pragma once
#include "memoryRegion.hpp"
#include <cstdint>

// A collection of nice functions to convert a binary stream / memoryRegion into
// something more legible

typedef unsigned char uchar;

// Convert a binary stream to a string of ascii chars, except all ascii vars
// that are not a readable character are changed to a dot.
std::string binaryStream2String(const BinaryStream &bs) {
  std::string str(bs.size(), '.');
  for (size_t i = 0; i < bs.size(); i++) {
    if (bs[i] < 32)
      continue;

    str[i] = bs[i];
  }
  return str;
}
