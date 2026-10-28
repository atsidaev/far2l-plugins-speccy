#pragma once
#include <windows.h>
#include "../shared/hrust.hpp"

inline bool dehrust(const BYTE * source, size_t size, BYTE * destination, size_t capacity,
                    size_t & written) {
  return speccy::hrust2(source, size, destination, capacity, written);
}

inline WORD crc16calc(const BYTE * source, size_t size) {
  return speccy::crc16(source, size);
}
