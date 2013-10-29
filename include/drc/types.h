// Definition of common typedefs used throughout libdrc.
//
// These are mostly short versions of <cstdint> standard types. These type
// definitions are constrained to the drc namespace to avoid polluting the
// common global namespace.

#pragma once

#include <cstdint>
#include <vector>

namespace drc {

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t byte;

typedef std::vector<byte> buffer;

}  // namespace drc
