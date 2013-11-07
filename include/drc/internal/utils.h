#pragma once

#include <drc/types.h>

namespace drc {

inline byte InsertBits(byte v, byte to_insert, int b, int e) {
  byte mask = ((1 << e) - 1) ^ ((1 << b) - 1);
  to_insert <<= b;
  to_insert &= mask;
  v &= ~mask;
  return v | to_insert;
}

inline byte ExtractBits(byte v, int b, int e) {
  byte mask = ((1 << e) - 1) ^ ((1 << b) - 1);
  return (v & mask) >> b;
}

}  // namespace drc
