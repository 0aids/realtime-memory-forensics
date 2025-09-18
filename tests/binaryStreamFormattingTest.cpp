#include "binaryStreamFormatting.hpp"
#include "logger.hpp"
#include "memoryRegion.hpp"

static inline BinaryStream string2BinaryStream(std::string str) {
  BinaryStream bs(str.begin(), str.end());
  return bs;
}

// NOT null terminated
BinaryStream cstring2BinaryStream(const char *cstr, size_t size) {
  BinaryStream bs(size);
  for (size_t i = 0; i < size; i++) {
    bs[i] = cstr[i];
  }
  return bs;
}

int main() {
  //
  const char *str = ""; // Perform an illgeal read for the lols
  const char *str1 = "Testing r testing 1 2 3";
  DEBUG(str);
  BinaryStream bs = cstring2BinaryStream(str1, 23);
  DEBUG("First conversion: \n\t" << binaryStream2String(bs));
  bs = cstring2BinaryStream(str, 40);
  DEBUG("");
  DEBUG("Second conversion (Reading some illegal part of memory): \n\t"
        << binaryStream2String(bs));
}
