#define RYU_OPTIMIZE_SIZE 1

#define bool _Bool

#include "go-ryu.h"
#include <inttypes.h>

// File common.h imported from ryu
// Copyright 2018 Ulf Adams
//
// The contents of this file may be used under the terms of the Apache License,
// Version 2.0.
//
//    (See accompanying file LICENSE-Apache or copy at
//     http://www.apache.org/licenses/LICENSE-2.0)
//
// Alternatively, the contents of this file may be used under the terms of
// the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE-Boost or copy at
//     https://www.boost.org/LICENSE_1_0.txt)
//
// Unless required by applicable law or agreed to in writing, this software
// is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.
#ifndef RYU_COMMON_H
#define RYU_COMMON_H

#include <assert.h>
#include <stdint.h>
#include <string.h>

#if defined(_M_IX86) || defined(_M_ARM)
#define RYU_32_BIT_PLATFORM
#endif

// Returns the number of decimal digits in v, which must not contain more than 9 digits.
static inline uint32_t decimalLength9(const uint32_t v) {
  // Function precondition: v is not a 10-digit number.
  // (go_ryu_f2s: 9 digits are sufficient for round-tripping.)
  // (go_ryu_d2fixed: We print 9-digit blocks.)
  assert(v < 1000000000);
  if (v >= 100000000) { return 9; }
  if (v >= 10000000) { return 8; }
  if (v >= 1000000) { return 7; }
  if (v >= 100000) { return 6; }
  if (v >= 10000) { return 5; }
  if (v >= 1000) { return 4; }
  if (v >= 100) { return 3; }
  if (v >= 10) { return 2; }
  return 1;
}

// Returns e == 0 ? 1 : [log_2(5^e)]; requires 0 <= e <= 3528.
static inline int32_t log2pow5(const int32_t e) {
  // This approximation works up to the point that the multiplication overflows at e = 3529.
  // If the multiplication were done in 64 bits, it would fail at 5^4004 which is just greater
  // than 2^9297.
  assert(e >= 0);
  assert(e <= 3528);
  return (int32_t) ((((uint32_t) e) * 1217359) >> 19);
}

// Returns e == 0 ? 1 : ceil(log_2(5^e)); requires 0 <= e <= 3528.
static inline int32_t pow5bits(const int32_t e) {
  // This approximation works up to the point that the multiplication overflows at e = 3529.
  // If the multiplication were done in 64 bits, it would fail at 5^4004 which is just greater
  // than 2^9297.
  assert(e >= 0);
  assert(e <= 3528);
  return (int32_t) (((((uint32_t) e) * 1217359) >> 19) + 1);
}

// Returns e == 0 ? 1 : ceil(log_2(5^e)); requires 0 <= e <= 3528.
static inline int32_t ceil_log2pow5(const int32_t e) {
  return log2pow5(e) + 1;
}

// Returns floor(log_10(2^e)); requires 0 <= e <= 1650.
static inline uint32_t log10Pow2(const int32_t e) {
  // The first value this approximation fails for is 2^1651 which is just greater than 10^297.
  assert(e >= 0);
  assert(e <= 1650);
  return (((uint32_t) e) * 78913) >> 18;
}

// Returns floor(log_10(5^e)); requires 0 <= e <= 2620.
static inline uint32_t log10Pow5(const int32_t e) {
  // The first value this approximation fails for is 5^2621 which is just greater than 10^1832.
  assert(e >= 0);
  assert(e <= 2620);
  return (((uint32_t) e) * 732923) >> 20;
}

static inline int copy_special_str(char * const result, const bool sign, const bool exponent, const bool mantissa) {
  if (mantissa) {
    if (sign) result[0] = '-';
    memcpy(result + sign, "nan", 3); // String changed to match musl's fmt_fp
    return sign + 3;
  }
  if (sign) {
    result[0] = '-';
  }
  if (exponent) {
    memcpy(result + sign, "inf", 3); // String changed to match musl's fmt_fp
    return sign + 3;
  }
  memcpy(result + sign, "0E0", 3);
  return sign + 3;
}

static inline uint32_t float_to_bits(const float f) {
  uint32_t bits = 0;
  memcpy(&bits, &f, sizeof(float));
  return bits;
}

static inline uint64_t double_to_bits(const double d) {
  uint64_t bits = 0;
  memcpy(&bits, &d, sizeof(double));
  return bits;
}

#endif // RYU_COMMON_H
// End of file common.h imported from ryu

// File digit_table.h imported from ryu
// Copyright 2018 Ulf Adams
//
// The contents of this file may be used under the terms of the Apache License,
// Version 2.0.
//
//    (See accompanying file LICENSE-Apache or copy at
//     http://www.apache.org/licenses/LICENSE-2.0)
//
// Alternatively, the contents of this file may be used under the terms of
// the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE-Boost or copy at
//     https://www.boost.org/LICENSE_1_0.txt)
//
// Unless required by applicable law or agreed to in writing, this software
// is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.
#ifndef RYU_DIGIT_TABLE_H
#define RYU_DIGIT_TABLE_H

// A table of all two-digit numbers. This is used to speed up decimal digit
// generation by copying pairs of digits into the final output.
static const char DIGIT_TABLE[200] = {
  '0','0','0','1','0','2','0','3','0','4','0','5','0','6','0','7','0','8','0','9',
  '1','0','1','1','1','2','1','3','1','4','1','5','1','6','1','7','1','8','1','9',
  '2','0','2','1','2','2','2','3','2','4','2','5','2','6','2','7','2','8','2','9',
  '3','0','3','1','3','2','3','3','3','4','3','5','3','6','3','7','3','8','3','9',
  '4','0','4','1','4','2','4','3','4','4','4','5','4','6','4','7','4','8','4','9',
  '5','0','5','1','5','2','5','3','5','4','5','5','5','6','5','7','5','8','5','9',
  '6','0','6','1','6','2','6','3','6','4','6','5','6','6','6','7','6','8','6','9',
  '7','0','7','1','7','2','7','3','7','4','7','5','7','6','7','7','7','8','7','9',
  '8','0','8','1','8','2','8','3','8','4','8','5','8','6','8','7','8','8','8','9',
  '9','0','9','1','9','2','9','3','9','4','9','5','9','6','9','7','9','8','9','9'
};

#endif // RYU_DIGIT_TABLE_H
// End of file digit_table.h imported from ryu

// File d2s_intrinsics.h imported from ryu
// Copyright 2018 Ulf Adams
//
// The contents of this file may be used under the terms of the Apache License,
// Version 2.0.
//
//    (See accompanying file LICENSE-Apache or copy at
//     http://www.apache.org/licenses/LICENSE-2.0)
//
// Alternatively, the contents of this file may be used under the terms of
// the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE-Boost or copy at
//     https://www.boost.org/LICENSE_1_0.txt)
//
// Unless required by applicable law or agreed to in writing, this software
// is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.
#ifndef RYU_D2S_INTRINSICS_H
#define RYU_D2S_INTRINSICS_H

#include <assert.h>
#include <stdint.h>

// Defines RYU_32_BIT_PLATFORM if applicable.

// ABSL avoids uint128_t on Win32 even if __SIZEOF_INT128__ is defined.
// Let's do the same for now.
#if defined(__SIZEOF_INT128__) && !defined(_MSC_VER) && !defined(RYU_ONLY_64_BIT_OPS)
#define HAS_UINT128
#elif defined(_MSC_VER) && !defined(RYU_ONLY_64_BIT_OPS) && defined(_M_X64)
#define HAS_64_BIT_INTRINSICS
#endif

#if defined(HAS_UINT128)
typedef __uint128_t uint128_t;
#endif

#if defined(HAS_64_BIT_INTRINSICS)

#include <intrin.h>

static inline uint64_t umul128(const uint64_t a, const uint64_t b, uint64_t* const productHi) {
  return _umul128(a, b, productHi);
}

// Returns the lower 64 bits of (hi*2^64 + lo) >> dist, with 0 < dist < 64.
static inline uint64_t shiftright128(const uint64_t lo, const uint64_t hi, const uint32_t dist) {
  // For the __shiftright128 intrinsic, the shift value is always
  // modulo 64.
  // In the current implementation of the double-precision version
  // of Ryu, the shift value is always < 64. (In the case
  // RYU_OPTIMIZE_SIZE == 0, the shift value is in the range [49, 58].
  // Otherwise in the range [2, 59].)
  // However, this function is now also called by s2d, which requires supporting
  // the larger shift range (TODO: what is the actual range?).
  // Check this here in case a future change requires larger shift
  // values. In this case this function needs to be adjusted.
  assert(dist < 64);
  return __shiftright128(lo, hi, (unsigned char) dist);
}

#else // defined(HAS_64_BIT_INTRINSICS)

static inline uint64_t umul128(const uint64_t a, const uint64_t b, uint64_t* const productHi) {
  // The casts here help MSVC to avoid calls to the __allmul library function.
  const uint32_t aLo = (uint32_t)a;
  const uint32_t aHi = (uint32_t)(a >> 32);
  const uint32_t bLo = (uint32_t)b;
  const uint32_t bHi = (uint32_t)(b >> 32);

  const uint64_t b00 = (uint64_t)aLo * bLo;
  const uint64_t b01 = (uint64_t)aLo * bHi;
  const uint64_t b10 = (uint64_t)aHi * bLo;
  const uint64_t b11 = (uint64_t)aHi * bHi;

  const uint32_t b00Lo = (uint32_t)b00;
  const uint32_t b00Hi = (uint32_t)(b00 >> 32);

  const uint64_t mid1 = b10 + b00Hi;
  const uint32_t mid1Lo = (uint32_t)(mid1);
  const uint32_t mid1Hi = (uint32_t)(mid1 >> 32);

  const uint64_t mid2 = b01 + mid1Lo;
  const uint32_t mid2Lo = (uint32_t)(mid2);
  const uint32_t mid2Hi = (uint32_t)(mid2 >> 32);

  const uint64_t pHi = b11 + mid1Hi + mid2Hi;
  const uint64_t pLo = ((uint64_t)mid2Lo << 32) | b00Lo;

  *productHi = pHi;
  return pLo;
}

static inline uint64_t shiftright128(const uint64_t lo, const uint64_t hi, const uint32_t dist) {
  // We don't need to handle the case dist >= 64 here (see above).
  assert(dist < 64);
  assert(dist > 0);
  return (hi << (64 - dist)) | (lo >> dist);
}

#endif // defined(HAS_64_BIT_INTRINSICS)

#if defined(RYU_32_BIT_PLATFORM)

// Returns the high 64 bits of the 128-bit product of a and b.
static inline uint64_t umulh(const uint64_t a, const uint64_t b) {
  // Reuse the umul128 implementation.
  // Optimizers will likely eliminate the instructions used to compute the
  // low part of the product.
  uint64_t hi;
  umul128(a, b, &hi);
  return hi;
}

// On 32-bit platforms, compilers typically generate calls to library
// functions for 64-bit divisions, even if the divisor is a constant.
//
// E.g.:
// https://bugs.llvm.org/show_bug.cgi?id=37932
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=17958
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=37443
//
// The functions here perform division-by-constant using multiplications
// in the same way as 64-bit compilers would do.
//
// NB:
// The multipliers and shift values are the ones generated by clang x64
// for expressions like x/5, x/10, etc.

static inline uint64_t div5(const uint64_t x) {
  return umulh(x, 0xCCCCCCCCCCCCCCCDu) >> 2;
}

static inline uint64_t div10(const uint64_t x) {
  return umulh(x, 0xCCCCCCCCCCCCCCCDu) >> 3;
}

static inline uint64_t div100(const uint64_t x) {
  return umulh(x >> 2, 0x28F5C28F5C28F5C3u) >> 2;
}

static inline uint64_t div1e8(const uint64_t x) {
  return umulh(x, 0xABCC77118461CEFDu) >> 26;
}

static inline uint64_t div1e9(const uint64_t x) {
  return umulh(x >> 9, 0x44B82FA09B5A53u) >> 11;
}

static inline uint32_t mod1e9(const uint64_t x) {
  // Avoid 64-bit math as much as possible.
  // Returning (uint32_t) (x - 1000000000 * div1e9(x)) would
  // perform 32x64-bit multiplication and 64-bit subtraction.
  // x and 1000000000 * div1e9(x) are guaranteed to differ by
  // less than 10^9, so their highest 32 bits must be identical,
  // so we can truncate both sides to uint32_t before subtracting.
  // We can also simplify (uint32_t) (1000000000 * div1e9(x)).
  // We can truncate before multiplying instead of after, as multiplying
  // the highest 32 bits of div1e9(x) can't affect the lowest 32 bits.
  return ((uint32_t) x) - 1000000000 * ((uint32_t) div1e9(x));
}

#else // defined(RYU_32_BIT_PLATFORM)

static inline uint64_t div5(const uint64_t x) {
  return x / 5;
}

static inline uint64_t div10(const uint64_t x) {
  return x / 10;
}

static inline uint64_t div100(const uint64_t x) {
  return x / 100;
}

static inline uint64_t div1e8(const uint64_t x) {
  return x / 100000000;
}

static inline uint64_t div1e9(const uint64_t x) {
  return x / 1000000000;
}

static inline uint32_t mod1e9(const uint64_t x) {
  return (uint32_t) (x - 1000000000 * div1e9(x));
}

#endif // defined(RYU_32_BIT_PLATFORM)

static inline uint32_t pow5Factor(uint64_t value) {
  const uint64_t m_inv_5 = 14757395258967641293u; // 5 * m_inv_5 = 1 (mod 2^64)
  const uint64_t n_div_5 = 3689348814741910323u;  // #{ n | n = 0 (mod 2^64) } = 2^64 / 5
  uint32_t count = 0;
  for (;;) {
    assert(value != 0);
    value *= m_inv_5;
    if (value > n_div_5)
      break;
    ++count;
  }
  return count;
}

// Returns true if value is divisible by 5^p.
static inline bool multipleOfPowerOf5(const uint64_t value, const uint32_t p) {
  // I tried a case distinction on p, but there was no performance difference.
  return pow5Factor(value) >= p;
}

// Returns true if value is divisible by 2^p.
static inline bool multipleOfPowerOf2(const uint64_t value, const uint32_t p) {
  assert(value != 0);
  assert(p < 64);
  // __builtin_ctzll doesn't appear to be faster here.
  return (value & ((1ull << p) - 1)) == 0;
}

// We need a 64x128-bit multiplication and a subsequent 128-bit shift.
// Multiplication:
//   The 64-bit factor is variable and passed in, the 128-bit factor comes
//   from a lookup table. We know that the 64-bit factor only has 55
//   significant bits (i.e., the 9 topmost bits are zeros). The 128-bit
//   factor only has 124 significant bits (i.e., the 4 topmost bits are
//   zeros).
// Shift:
//   In principle, the multiplication result requires 55 + 124 = 179 bits to
//   represent. However, we then shift this value to the right by j, which is
//   at least j >= 115, so the result is guaranteed to fit into 179 - 115 = 64
//   bits. This means that we only need the topmost 64 significant bits of
//   the 64x128-bit multiplication.
//
// There are several ways to do this:
// 1. Best case: the compiler exposes a 128-bit type.
//    We perform two 64x64-bit multiplications, add the higher 64 bits of the
//    lower result to the higher result, and shift by j - 64 bits.
//
//    We explicitly cast from 64-bit to 128-bit, so the compiler can tell
//    that these are only 64-bit inputs, and can map these to the best
//    possible sequence of assembly instructions.
//    x64 machines happen to have matching assembly instructions for
//    64x64-bit multiplications and 128-bit shifts.
//
// 2. Second best case: the compiler exposes intrinsics for the x64 assembly
//    instructions mentioned in 1.
//
// 3. We only have 64x64 bit instructions that return the lower 64 bits of
//    the result, i.e., we have to use plain C.
//    Our inputs are less than the full width, so we have three options:
//    a. Ignore this fact and just implement the intrinsics manually.
//    b. Split both into 31-bit pieces, which guarantees no internal overflow,
//       but requires extra work upfront (unless we change the lookup table).
//    c. Split only the first factor into 31-bit pieces, which also guarantees
//       no internal overflow, but requires extra work since the intermediate
//       results are not perfectly aligned.
#if defined(HAS_UINT128)

// Best case: use 128-bit type.
static inline uint64_t mulShift64(const uint64_t m, const uint64_t* const mul, const int32_t j) {
  const uint128_t b0 = ((uint128_t) m) * mul[0];
  const uint128_t b2 = ((uint128_t) m) * mul[1];
  return (uint64_t) (((b0 >> 64) + b2) >> (j - 64));
}

static inline uint64_t mulShiftAll64(const uint64_t m, const uint64_t* const mul, const int32_t j,
  uint64_t* const vp, uint64_t* const vm, const uint32_t mmShift) {
//  m <<= 2;
//  uint128_t b0 = ((uint128_t) m) * mul[0]; // 0
//  uint128_t b2 = ((uint128_t) m) * mul[1]; // 64
//
//  uint128_t hi = (b0 >> 64) + b2;
//  uint128_t lo = b0 & 0xffffffffffffffffull;
//  uint128_t factor = (((uint128_t) mul[1]) << 64) + mul[0];
//  uint128_t vpLo = lo + (factor << 1);
//  *vp = (uint64_t) ((hi + (vpLo >> 64)) >> (j - 64));
//  uint128_t vmLo = lo - (factor << mmShift);
//  *vm = (uint64_t) ((hi + (vmLo >> 64) - (((uint128_t) 1ull) << 64)) >> (j - 64));
//  return (uint64_t) (hi >> (j - 64));
  *vp = mulShift64(4 * m + 2, mul, j);
  *vm = mulShift64(4 * m - 1 - mmShift, mul, j);
  return mulShift64(4 * m, mul, j);
}

#elif defined(HAS_64_BIT_INTRINSICS)

static inline uint64_t mulShift64(const uint64_t m, const uint64_t* const mul, const int32_t j) {
  // m is maximum 55 bits
  uint64_t high1;                                   // 128
  const uint64_t low1 = umul128(m, mul[1], &high1); // 64
  uint64_t high0;                                   // 64
  umul128(m, mul[0], &high0);                       // 0
  const uint64_t sum = high0 + low1;
  if (sum < high0) {
    ++high1; // overflow into high1
  }
  return shiftright128(sum, high1, j - 64);
}

static inline uint64_t mulShiftAll64(const uint64_t m, const uint64_t* const mul, const int32_t j,
  uint64_t* const vp, uint64_t* const vm, const uint32_t mmShift) {
  *vp = mulShift64(4 * m + 2, mul, j);
  *vm = mulShift64(4 * m - 1 - mmShift, mul, j);
  return mulShift64(4 * m, mul, j);
}

#else // !defined(HAS_UINT128) && !defined(HAS_64_BIT_INTRINSICS)

static inline uint64_t mulShift64(const uint64_t m, const uint64_t* const mul, const int32_t j) {
  // m is maximum 55 bits
  uint64_t high1;                                   // 128
  const uint64_t low1 = umul128(m, mul[1], &high1); // 64
  uint64_t high0;                                   // 64
  umul128(m, mul[0], &high0);                       // 0
  const uint64_t sum = high0 + low1;
  if (sum < high0) {
    ++high1; // overflow into high1
  }
  return shiftright128(sum, high1, j - 64);
}

// This is faster if we don't have a 64x64->128-bit multiplication.
static inline uint64_t mulShiftAll64(uint64_t m, const uint64_t* const mul, const int32_t j,
  uint64_t* const vp, uint64_t* const vm, const uint32_t mmShift) {
  m <<= 1;
  // m is maximum 55 bits
  uint64_t tmp;
  const uint64_t lo = umul128(m, mul[0], &tmp);
  uint64_t hi;
  const uint64_t mid = tmp + umul128(m, mul[1], &hi);
  hi += mid < tmp; // overflow into hi

  const uint64_t lo2 = lo + mul[0];
  const uint64_t mid2 = mid + mul[1] + (lo2 < lo);
  const uint64_t hi2 = hi + (mid2 < mid);
  *vp = shiftright128(mid2, hi2, (uint32_t) (j - 64 - 1));

  if (mmShift == 1) {
    const uint64_t lo3 = lo - mul[0];
    const uint64_t mid3 = mid - mul[1] - (lo3 > lo);
    const uint64_t hi3 = hi - (mid3 > mid);
    *vm = shiftright128(mid3, hi3, (uint32_t) (j - 64 - 1));
  } else {
    const uint64_t lo3 = lo + lo;
    const uint64_t mid3 = mid + mid + (lo3 < lo);
    const uint64_t hi3 = hi + hi + (mid3 < mid);
    const uint64_t lo4 = lo3 - mul[0];
    const uint64_t mid4 = mid3 - mul[1] - (lo4 > lo3);
    const uint64_t hi4 = hi3 - (mid4 > mid3);
    *vm = shiftright128(mid4, hi4, (uint32_t) (j - 64));
  }

  return shiftright128(mid, hi, (uint32_t) (j - 64 - 1));
}

#endif // HAS_64_BIT_INTRINSICS

#endif // RYU_D2S_INTRINSICS_H
// End of file d2s_intrinsics.h imported from ryu

// File d2s_small_table.h imported from ryu
// Copyright 2018 Ulf Adams
//
// The contents of this file may be used under the terms of the Apache License,
// Version 2.0.
//
//    (See accompanying file LICENSE-Apache or copy at
//     http://www.apache.org/licenses/LICENSE-2.0)
//
// Alternatively, the contents of this file may be used under the terms of
// the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE-Boost or copy at
//     https://www.boost.org/LICENSE_1_0.txt)
//
// Unless required by applicable law or agreed to in writing, this software
// is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.
#ifndef RYU_D2S_SMALL_TABLE_H
#define RYU_D2S_SMALL_TABLE_H

// Defines HAS_UINT128 and uint128_t if applicable.

// These tables are generated by PrintDoubleLookupTable.
#define DOUBLE_POW5_INV_BITCOUNT 125
#define DOUBLE_POW5_BITCOUNT 125

static const uint64_t DOUBLE_POW5_INV_SPLIT2[15][2] = {
  {                    1u, 2305843009213693952u },
  {  5955668970331000884u, 1784059615882449851u },
  {  8982663654677661702u, 1380349269358112757u },
  {  7286864317269821294u, 2135987035920910082u },
  {  7005857020398200553u, 1652639921975621497u },
  { 17965325103354776697u, 1278668206209430417u },
  {  8928596168509315048u, 1978643211784836272u },
  { 10075671573058298858u, 1530901034580419511u },
  {   597001226353042382u, 1184477304306571148u },
  {  1527430471115325346u, 1832889850782397517u },
  { 12533209867169019542u, 1418129833677084982u },
  {  5577825024675947042u, 2194449627517475473u },
  { 11006974540203867551u, 1697873161311732311u },
  { 10313493231639821582u, 1313665730009899186u },
  { 12701016819766672773u, 2032799256770390445u }
};
static const uint32_t POW5_INV_OFFSETS[19] = {
  0x54544554, 0x04055545, 0x10041000, 0x00400414, 0x40010000, 0x41155555,
  0x00000454, 0x00010044, 0x40000000, 0x44000041, 0x50454450, 0x55550054,
  0x51655554, 0x40004000, 0x01000001, 0x00010500, 0x51515411, 0x05555554,
  0x00000000
};

static const uint64_t DOUBLE_POW5_SPLIT2[13][2] = {
  {                    0u, 1152921504606846976u },
  {                    0u, 1490116119384765625u },
  {  1032610780636961552u, 1925929944387235853u },
  {  7910200175544436838u, 1244603055572228341u },
  { 16941905809032713930u, 1608611746708759036u },
  { 13024893955298202172u, 2079081953128979843u },
  {  6607496772837067824u, 1343575221513417750u },
  { 17332926989895652603u, 1736530273035216783u },
  { 13037379183483547984u, 2244412773384604712u },
  {  1605989338741628675u, 1450417759929778918u },
  {  9630225068416591280u, 1874621017369538693u },
  {   665883850346957067u, 1211445438634777304u },
  { 14931890668723713708u, 1565756531257009982u }
};
static const uint32_t POW5_OFFSETS[21] = {
  0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x40000000, 0x59695995,
  0x55545555, 0x56555515, 0x41150504, 0x40555410, 0x44555145, 0x44504540,
  0x45555550, 0x40004000, 0x96440440, 0x55565565, 0x54454045, 0x40154151,
  0x55559155, 0x51405555, 0x00000105
};

#define POW5_TABLE_SIZE 26
static const uint64_t DOUBLE_POW5_TABLE[POW5_TABLE_SIZE] = {
1ull, 5ull, 25ull, 125ull, 625ull, 3125ull, 15625ull, 78125ull, 390625ull,
1953125ull, 9765625ull, 48828125ull, 244140625ull, 1220703125ull, 6103515625ull,
30517578125ull, 152587890625ull, 762939453125ull, 3814697265625ull,
19073486328125ull, 95367431640625ull, 476837158203125ull,
2384185791015625ull, 11920928955078125ull, 59604644775390625ull,
298023223876953125ull //, 1490116119384765625ull
};

#if defined(HAS_UINT128)

// Computes 5^i in the form required by Ryu, and stores it in the given pointer.
static inline void double_computePow5(const uint32_t i, uint64_t* const result) {
  const uint32_t base = i / POW5_TABLE_SIZE;
  const uint32_t base2 = base * POW5_TABLE_SIZE;
  const uint32_t offset = i - base2;
  const uint64_t* const mul = DOUBLE_POW5_SPLIT2[base];
  if (offset == 0) {
    result[0] = mul[0];
    result[1] = mul[1];
    return;
  }
  const uint64_t m = DOUBLE_POW5_TABLE[offset];
  const uint128_t b0 = ((uint128_t) m) * mul[0];
  const uint128_t b2 = ((uint128_t) m) * mul[1];
  const uint32_t delta = pow5bits(i) - pow5bits(base2);
  const uint128_t shiftedSum = (b0 >> delta) + (b2 << (64 - delta)) + ((POW5_OFFSETS[i / 16] >> ((i % 16) << 1)) & 3);
  result[0] = (uint64_t) shiftedSum;
  result[1] = (uint64_t) (shiftedSum >> 64);
}

// Computes 5^-i in the form required by Ryu, and stores it in the given pointer.
static inline void double_computeInvPow5(const uint32_t i, uint64_t* const result) {
  const uint32_t base = (i + POW5_TABLE_SIZE - 1) / POW5_TABLE_SIZE;
  const uint32_t base2 = base * POW5_TABLE_SIZE;
  const uint32_t offset = base2 - i;
  const uint64_t* const mul = DOUBLE_POW5_INV_SPLIT2[base]; // 1/5^base2
  if (offset == 0) {
    result[0] = mul[0];
    result[1] = mul[1];
    return;
  }
  const uint64_t m = DOUBLE_POW5_TABLE[offset]; // 5^offset
  const uint128_t b0 = ((uint128_t) m) * (mul[0] - 1);
  const uint128_t b2 = ((uint128_t) m) * mul[1]; // 1/5^base2 * 5^offset = 1/5^(base2-offset) = 1/5^i
  const uint32_t delta = pow5bits(base2) - pow5bits(i);
  const uint128_t shiftedSum =
    ((b0 >> delta) + (b2 << (64 - delta))) + 1 + ((POW5_INV_OFFSETS[i / 16] >> ((i % 16) << 1)) & 3);
  result[0] = (uint64_t) shiftedSum;
  result[1] = (uint64_t) (shiftedSum >> 64);
}

#else // defined(HAS_UINT128)

// Computes 5^i in the form required by Ryu, and stores it in the given pointer.
static inline void double_computePow5(const uint32_t i, uint64_t* const result) {
  const uint32_t base = i / POW5_TABLE_SIZE;
  const uint32_t base2 = base * POW5_TABLE_SIZE;
  const uint32_t offset = i - base2;
  const uint64_t* const mul = DOUBLE_POW5_SPLIT2[base];
  if (offset == 0) {
    result[0] = mul[0];
    result[1] = mul[1];
    return;
  }
  const uint64_t m = DOUBLE_POW5_TABLE[offset];
  uint64_t high1;
  const uint64_t low1 = umul128(m, mul[1], &high1);
  uint64_t high0;
  const uint64_t low0 = umul128(m, mul[0], &high0);
  const uint64_t sum = high0 + low1;
  if (sum < high0) {
    ++high1; // overflow into high1
  }
  // high1 | sum | low0
  const uint32_t delta = pow5bits(i) - pow5bits(base2);
  result[0] = shiftright128(low0, sum, delta) + ((POW5_OFFSETS[i / 16] >> ((i % 16) << 1)) & 3);
  result[1] = shiftright128(sum, high1, delta);
}

// Computes 5^-i in the form required by Ryu, and stores it in the given pointer.
static inline void double_computeInvPow5(const uint32_t i, uint64_t* const result) {
  const uint32_t base = (i + POW5_TABLE_SIZE - 1) / POW5_TABLE_SIZE;
  const uint32_t base2 = base * POW5_TABLE_SIZE;
  const uint32_t offset = base2 - i;
  const uint64_t* const mul = DOUBLE_POW5_INV_SPLIT2[base]; // 1/5^base2
  if (offset == 0) {
    result[0] = mul[0];
    result[1] = mul[1];
    return;
  }
  const uint64_t m = DOUBLE_POW5_TABLE[offset];
  uint64_t high1;
  const uint64_t low1 = umul128(m, mul[1], &high1);
  uint64_t high0;
  const uint64_t low0 = umul128(m, mul[0] - 1, &high0);
  const uint64_t sum = high0 + low1;
  if (sum < high0) {
    ++high1; // overflow into high1
  }
  // high1 | sum | low0
  const uint32_t delta = pow5bits(base2) - pow5bits(i);
  result[0] = shiftright128(low0, sum, delta) + 1 + ((POW5_INV_OFFSETS[i / 16] >> ((i % 16) << 1)) & 3);
  result[1] = shiftright128(sum, high1, delta);
}

#endif // defined(HAS_UINT128)

#endif // RYU_D2S_SMALL_TABLE_H
// End of file d2s_small_table.h imported from ryu

// File d2s.c imported from ryu
// Copyright 2018 Ulf Adams
//
// The contents of this file may be used under the terms of the Apache License,
// Version 2.0.
//
//    (See accompanying file LICENSE-Apache or copy at
//     http://www.apache.org/licenses/LICENSE-2.0)
//
// Alternatively, the contents of this file may be used under the terms of
// the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE-Boost or copy at
//     https://www.boost.org/LICENSE_1_0.txt)
//
// Unless required by applicable law or agreed to in writing, this software
// is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.

// Runtime compiler options:
// -DRYU_DEBUG Generate verbose debugging output to stdout.
//
// -DRYU_ONLY_64_BIT_OPS Avoid using uint128_t or 64-bit intrinsics. Slower,
//     depending on your compiler.
//
// -DRYU_OPTIMIZE_SIZE Use smaller lookup tables. Instead of storing every
//     required power of 5, only store every 26th entry, and compute
//     intermediate values with a multiplication. This reduces the lookup table
//     size by about 10x (only one case, and only double) at the cost of some
//     performance. Currently requires MSVC intrinsics.


#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef RYU_DEBUG
#include <inttypes.h>
#include <stdio.h>
#endif


// Include either the small or the full lookup tables depending on the mode.
#if defined(RYU_OPTIMIZE_SIZE)
#else
#endif

#define DOUBLE_MANTISSA_BITS 52
#define DOUBLE_EXPONENT_BITS 11
#define DOUBLE_BIAS 1023

static inline uint32_t decimalLength17(const uint64_t v) {
  // This is slightly faster than a loop.
  // The average output length is 16.38 digits, so we check high-to-low.
  // Function precondition: v is not an 18, 19, or 20-digit number.
  // (17 digits are sufficient for round-tripping.)
  assert(v < 100000000000000000L);
  if (v >= 10000000000000000L) { return 17; }
  if (v >= 1000000000000000L) { return 16; }
  if (v >= 100000000000000L) { return 15; }
  if (v >= 10000000000000L) { return 14; }
  if (v >= 1000000000000L) { return 13; }
  if (v >= 100000000000L) { return 12; }
  if (v >= 10000000000L) { return 11; }
  if (v >= 1000000000L) { return 10; }
  if (v >= 100000000L) { return 9; }
  if (v >= 10000000L) { return 8; }
  if (v >= 1000000L) { return 7; }
  if (v >= 100000L) { return 6; }
  if (v >= 10000L) { return 5; }
  if (v >= 1000L) { return 4; }
  if (v >= 100L) { return 3; }
  if (v >= 10L) { return 2; }
  return 1;
}

// A floating decimal representing m * 10^e.
typedef struct floating_decimal_64 {
  uint64_t mantissa;
  // Decimal exponent's range is -324 to 308
  // inclusive, and can fit in a short if needed.
  int32_t exponent;
} floating_decimal_64;

static inline floating_decimal_64 d2d(const uint64_t ieeeMantissa, const uint32_t ieeeExponent) {
  int32_t e2;
  uint64_t m2;
  if (ieeeExponent == 0) {
    // We subtract 2 so that the bounds computation has 2 additional bits.
    e2 = 1 - DOUBLE_BIAS - DOUBLE_MANTISSA_BITS - 2;
    m2 = ieeeMantissa;
  } else {
    e2 = (int32_t) ieeeExponent - DOUBLE_BIAS - DOUBLE_MANTISSA_BITS - 2;
    m2 = (1ull << DOUBLE_MANTISSA_BITS) | ieeeMantissa;
  }
  const bool even = (m2 & 1) == 0;
  const bool acceptBounds = even;

#ifdef RYU_DEBUG
  printf("-> %" PRIu64 " * 2^%d\n", m2, e2 + 2);
#endif

  // Step 2: Determine the interval of valid decimal representations.
  const uint64_t mv = 4 * m2;
  // Implicit bool -> int conversion. True is 1, false is 0.
  const uint32_t mmShift = ieeeMantissa != 0 || ieeeExponent <= 1;
  // We would compute mp and mm like this:
  // uint64_t mp = 4 * m2 + 2;
  // uint64_t mm = mv - 1 - mmShift;

  // Step 3: Convert to a decimal power base using 128-bit arithmetic.
  uint64_t vr, vp, vm;
  int32_t e10;
  bool vmIsTrailingZeros = false;
  bool vrIsTrailingZeros = false;
  if (e2 >= 0) {
    // I tried special-casing q == 0, but there was no effect on performance.
    // This expression is slightly faster than max(0, log10Pow2(e2) - 1).
    const uint32_t q = log10Pow2(e2) - (e2 > 3);
    e10 = (int32_t) q;
    const int32_t k = DOUBLE_POW5_INV_BITCOUNT + pow5bits((int32_t) q) - 1;
    const int32_t i = -e2 + (int32_t) q + k;
#if defined(RYU_OPTIMIZE_SIZE)
    uint64_t pow5[2];
    double_computeInvPow5(q, pow5);
    vr = mulShiftAll64(m2, pow5, i, &vp, &vm, mmShift);
#else
    vr = mulShiftAll64(m2, DOUBLE_POW5_INV_SPLIT[q], i, &vp, &vm, mmShift);
#endif
#ifdef RYU_DEBUG
    printf("%" PRIu64 " * 2^%d / 10^%u\n", mv, e2, q);
    printf("V+=%" PRIu64 "\nV =%" PRIu64 "\nV-=%" PRIu64 "\n", vp, vr, vm);
#endif
    if (q <= 21) {
      // This should use q <= 22, but I think 21 is also safe. Smaller values
      // may still be safe, but it's more difficult to reason about them.
      // Only one of mp, mv, and mm can be a multiple of 5, if any.
      const uint32_t mvMod5 = ((uint32_t) mv) - 5 * ((uint32_t) div5(mv));
      if (mvMod5 == 0) {
        vrIsTrailingZeros = multipleOfPowerOf5(mv, q);
      } else if (acceptBounds) {
        // Same as min(e2 + (~mm & 1), pow5Factor(mm)) >= q
        // <=> e2 + (~mm & 1) >= q && pow5Factor(mm) >= q
        // <=> true && pow5Factor(mm) >= q, since e2 >= q.
        vmIsTrailingZeros = multipleOfPowerOf5(mv - 1 - mmShift, q);
      } else {
        // Same as min(e2 + 1, pow5Factor(mp)) >= q.
        vp -= multipleOfPowerOf5(mv + 2, q);
      }
    }
  } else {
    // This expression is slightly faster than max(0, log10Pow5(-e2) - 1).
    const uint32_t q = log10Pow5(-e2) - (-e2 > 1);
    e10 = (int32_t) q + e2;
    const int32_t i = -e2 - (int32_t) q;
    const int32_t k = pow5bits(i) - DOUBLE_POW5_BITCOUNT;
    const int32_t j = (int32_t) q - k;
#if defined(RYU_OPTIMIZE_SIZE)
    uint64_t pow5[2];
    double_computePow5(i, pow5);
    vr = mulShiftAll64(m2, pow5, j, &vp, &vm, mmShift);
#else
    vr = mulShiftAll64(m2, DOUBLE_POW5_SPLIT[i], j, &vp, &vm, mmShift);
#endif
#ifdef RYU_DEBUG
    printf("%" PRIu64 " * 5^%d / 10^%u\n", mv, -e2, q);
    printf("%u %d %d %d\n", q, i, k, j);
    printf("V+=%" PRIu64 "\nV =%" PRIu64 "\nV-=%" PRIu64 "\n", vp, vr, vm);
#endif
    if (q <= 1) {
      // {vr,vp,vm} is trailing zeros if {mv,mp,mm} has at least q trailing 0 bits.
      // mv = 4 * m2, so it always has at least two trailing 0 bits.
      vrIsTrailingZeros = true;
      if (acceptBounds) {
        // mm = mv - 1 - mmShift, so it has 1 trailing 0 bit iff mmShift == 1.
        vmIsTrailingZeros = mmShift == 1;
      } else {
        // mp = mv + 2, so it always has at least one trailing 0 bit.
        --vp;
      }
    } else if (q < 63) { // TODO(ulfjack): Use a tighter bound here.
      // We want to know if the full product has at least q trailing zeros.
      // We need to compute min(p2(mv), p5(mv) - e2) >= q
      // <=> p2(mv) >= q && p5(mv) - e2 >= q
      // <=> p2(mv) >= q (because -e2 >= q)
      vrIsTrailingZeros = multipleOfPowerOf2(mv, q);
#ifdef RYU_DEBUG
      printf("vr is trailing zeros=%s\n", vrIsTrailingZeros ? "true" : "false");
#endif
    }
  }
#ifdef RYU_DEBUG
  printf("e10=%d\n", e10);
  printf("V+=%" PRIu64 "\nV =%" PRIu64 "\nV-=%" PRIu64 "\n", vp, vr, vm);
  printf("vm is trailing zeros=%s\n", vmIsTrailingZeros ? "true" : "false");
  printf("vr is trailing zeros=%s\n", vrIsTrailingZeros ? "true" : "false");
#endif

  // Step 4: Find the shortest decimal representation in the interval of valid representations.
  int32_t removed = 0;
  uint8_t lastRemovedDigit = 0;
  uint64_t output;
  // On average, we remove ~2 digits.
  if (vmIsTrailingZeros || vrIsTrailingZeros) {
    // General case, which happens rarely (~0.7%).
    for (;;) {
      const uint64_t vpDiv10 = div10(vp);
      const uint64_t vmDiv10 = div10(vm);
      if (vpDiv10 <= vmDiv10) {
        break;
      }
      const uint32_t vmMod10 = ((uint32_t) vm) - 10 * ((uint32_t) vmDiv10);
      const uint64_t vrDiv10 = div10(vr);
      const uint32_t vrMod10 = ((uint32_t) vr) - 10 * ((uint32_t) vrDiv10);
      vmIsTrailingZeros &= vmMod10 == 0;
      vrIsTrailingZeros &= lastRemovedDigit == 0;
      lastRemovedDigit = (uint8_t) vrMod10;
      vr = vrDiv10;
      vp = vpDiv10;
      vm = vmDiv10;
      ++removed;
    }
#ifdef RYU_DEBUG
    printf("V+=%" PRIu64 "\nV =%" PRIu64 "\nV-=%" PRIu64 "\n", vp, vr, vm);
    printf("d-10=%s\n", vmIsTrailingZeros ? "true" : "false");
#endif
    if (vmIsTrailingZeros) {
      for (;;) {
        const uint64_t vmDiv10 = div10(vm);
        const uint32_t vmMod10 = ((uint32_t) vm) - 10 * ((uint32_t) vmDiv10);
        if (vmMod10 != 0) {
          break;
        }
        const uint64_t vpDiv10 = div10(vp);
        const uint64_t vrDiv10 = div10(vr);
        const uint32_t vrMod10 = ((uint32_t) vr) - 10 * ((uint32_t) vrDiv10);
        vrIsTrailingZeros &= lastRemovedDigit == 0;
        lastRemovedDigit = (uint8_t) vrMod10;
        vr = vrDiv10;
        vp = vpDiv10;
        vm = vmDiv10;
        ++removed;
      }
    }
#ifdef RYU_DEBUG
    printf("%" PRIu64 " %d\n", vr, lastRemovedDigit);
    printf("vr is trailing zeros=%s\n", vrIsTrailingZeros ? "true" : "false");
#endif
    if (vrIsTrailingZeros && lastRemovedDigit == 5 && vr % 2 == 0) {
      // Round even if the exact number is .....50..0.
      lastRemovedDigit = 4;
    }
    // We need to take vr + 1 if vr is outside bounds or we need to round up.
    output = vr + ((vr == vm && (!acceptBounds || !vmIsTrailingZeros)) || lastRemovedDigit >= 5);
  } else {
    // Specialized for the common case (~99.3%). Percentages below are relative to this.
    bool roundUp = false;
    const uint64_t vpDiv100 = div100(vp);
    const uint64_t vmDiv100 = div100(vm);
    if (vpDiv100 > vmDiv100) { // Optimization: remove two digits at a time (~86.2%).
      const uint64_t vrDiv100 = div100(vr);
      const uint32_t vrMod100 = ((uint32_t) vr) - 100 * ((uint32_t) vrDiv100);
      roundUp = vrMod100 >= 50;
      vr = vrDiv100;
      vp = vpDiv100;
      vm = vmDiv100;
      removed += 2;
    }
    // Loop iterations below (approximately), without optimization above:
    // 0: 0.03%, 1: 13.8%, 2: 70.6%, 3: 14.0%, 4: 1.40%, 5: 0.14%, 6+: 0.02%
    // Loop iterations below (approximately), with optimization above:
    // 0: 70.6%, 1: 27.8%, 2: 1.40%, 3: 0.14%, 4+: 0.02%
    for (;;) {
      const uint64_t vpDiv10 = div10(vp);
      const uint64_t vmDiv10 = div10(vm);
      if (vpDiv10 <= vmDiv10) {
        break;
      }
      const uint64_t vrDiv10 = div10(vr);
      const uint32_t vrMod10 = ((uint32_t) vr) - 10 * ((uint32_t) vrDiv10);
      roundUp = vrMod10 >= 5;
      vr = vrDiv10;
      vp = vpDiv10;
      vm = vmDiv10;
      ++removed;
    }
#ifdef RYU_DEBUG
    printf("%" PRIu64 " roundUp=%s\n", vr, roundUp ? "true" : "false");
    printf("vr is trailing zeros=%s\n", vrIsTrailingZeros ? "true" : "false");
#endif
    // We need to take vr + 1 if vr is outside bounds or we need to round up.
    output = vr + (vr == vm || roundUp);
  }
  const int32_t exp = e10 + removed;

#ifdef RYU_DEBUG
  printf("V+=%" PRIu64 "\nV =%" PRIu64 "\nV-=%" PRIu64 "\n", vp, vr, vm);
  printf("O=%" PRIu64 "\n", output);
  printf("EXP=%d\n", exp);
#endif

  floating_decimal_64 fd;
  fd.exponent = exp;
  fd.mantissa = output;
  return fd;
}

static inline int to_chars(const floating_decimal_64 v, const bool sign, char* const result) {
  // Step 5: Print the decimal representation.
  int index = 0;
  if (sign) {
    result[index++] = '-';
  }

  uint64_t output = v.mantissa;
  const uint32_t olength = decimalLength17(output);

#ifdef RYU_DEBUG
  printf("DIGITS=%" PRIu64 "\n", v.mantissa);
  printf("OLEN=%u\n", olength);
  printf("EXP=%u\n", v.exponent + olength);
#endif

  // Print the decimal digits.
  // The following code is equivalent to:
  // for (uint32_t i = 0; i < olength - 1; ++i) {
  //   const uint32_t c = output % 10; output /= 10;
  //   result[index + olength - i] = (char) ('0' + c);
  // }
  // result[index] = '0' + output % 10;

  uint32_t i = 0;
  // We prefer 32-bit operations, even on 64-bit platforms.
  // We have at most 17 digits, and uint32_t can store 9 digits.
  // If output doesn't fit into uint32_t, we cut off 8 digits,
  // so the rest will fit into uint32_t.
  if ((output >> 32) != 0) {
    // Expensive 64-bit division.
    const uint64_t q = div1e8(output);
    uint32_t output2 = ((uint32_t) output) - 100000000 * ((uint32_t) q);
    output = q;

    const uint32_t c = output2 % 10000;
    output2 /= 10000;
    const uint32_t d = output2 % 10000;
    const uint32_t c0 = (c % 100) << 1;
    const uint32_t c1 = (c / 100) << 1;
    const uint32_t d0 = (d % 100) << 1;
    const uint32_t d1 = (d / 100) << 1;
    memcpy(result + index + olength - i - 1, DIGIT_TABLE + c0, 2);
    memcpy(result + index + olength - i - 3, DIGIT_TABLE + c1, 2);
    memcpy(result + index + olength - i - 5, DIGIT_TABLE + d0, 2);
    memcpy(result + index + olength - i - 7, DIGIT_TABLE + d1, 2);
    i += 8;
  }
  uint32_t output2 = (uint32_t) output;
  while (output2 >= 10000) {
#ifdef __clang__ // https://bugs.llvm.org/show_bug.cgi?id=38217
    const uint32_t c = output2 - 10000 * (output2 / 10000);
#else
    const uint32_t c = output2 % 10000;
#endif
    output2 /= 10000;
    const uint32_t c0 = (c % 100) << 1;
    const uint32_t c1 = (c / 100) << 1;
    memcpy(result + index + olength - i - 1, DIGIT_TABLE + c0, 2);
    memcpy(result + index + olength - i - 3, DIGIT_TABLE + c1, 2);
    i += 4;
  }
  if (output2 >= 100) {
    const uint32_t c = (output2 % 100) << 1;
    output2 /= 100;
    memcpy(result + index + olength - i - 1, DIGIT_TABLE + c, 2);
    i += 2;
  }
  if (output2 >= 10) {
    const uint32_t c = output2 << 1;
    // We can't use memcpy here: the decimal dot goes between these two digits.
    result[index + olength - i] = DIGIT_TABLE[c + 1];
    result[index] = DIGIT_TABLE[c];
  } else {
    result[index] = (char) ('0' + output2);
  }

  // Print decimal point if needed.
  if (olength > 1) {
    result[index + 1] = '.';
    index += olength + 1;
  } else {
    ++index;
  }

  // Print the exponent.
  result[index++] = 'E';
  int32_t exp = v.exponent + (int32_t) olength - 1;
  if (exp < 0) {
    result[index++] = '-';
    exp = -exp;
  }

  if (exp >= 100) {
    const int32_t c = exp % 10;
    memcpy(result + index, DIGIT_TABLE + 2 * (exp / 10), 2);
    result[index + 2] = (char) ('0' + c);
    index += 3;
  } else if (exp >= 10) {
    memcpy(result + index, DIGIT_TABLE + 2 * exp, 2);
    index += 2;
  } else {
    result[index++] = (char) ('0' + exp);
  }

  return index;
}

static inline bool d2d_small_int(const uint64_t ieeeMantissa, const uint32_t ieeeExponent,
  floating_decimal_64* const v) {
  const uint64_t m2 = (1ull << DOUBLE_MANTISSA_BITS) | ieeeMantissa;
  const int32_t e2 = (int32_t) ieeeExponent - DOUBLE_BIAS - DOUBLE_MANTISSA_BITS;

  if (e2 > 0) {
    // f = m2 * 2^e2 >= 2^53 is an integer.
    // Ignore this case for now.
    return false;
  }

  if (e2 < -52) {
    // f < 1.
    return false;
  }

  // Since 2^52 <= m2 < 2^53 and 0 <= -e2 <= 52: 1 <= f = m2 / 2^-e2 < 2^53.
  // Test if the lower -e2 bits of the significand are 0, i.e. whether the fraction is 0.
  const uint64_t mask = (1ull << -e2) - 1;
  const uint64_t fraction = m2 & mask;
  if (fraction != 0) {
    return false;
  }

  // f is an integer in the range [1, 2^53).
  // Note: mantissa might contain trailing (decimal) 0's.
  // Note: since 2^53 < 10^16, there is no need to adjust decimalLength17().
  v->mantissa = m2 >> -e2;
  v->exponent = 0;
  return true;
}

int go_ryu_d2s_buffered_n(double f, char* result) {
  // Step 1: Decode the floating-point number, and unify normalized and subnormal cases.
  const uint64_t bits = double_to_bits(f);

#ifdef RYU_DEBUG
  printf("IN=");
  for (int32_t bit = 63; bit >= 0; --bit) {
    printf("%d", (int) ((bits >> bit) & 1));
  }
  printf("\n");
#endif

  // Decode bits into sign, mantissa, and exponent.
  const bool ieeeSign = ((bits >> (DOUBLE_MANTISSA_BITS + DOUBLE_EXPONENT_BITS)) & 1) != 0;
  const uint64_t ieeeMantissa = bits & ((1ull << DOUBLE_MANTISSA_BITS) - 1);
  const uint32_t ieeeExponent = (uint32_t) ((bits >> DOUBLE_MANTISSA_BITS) & ((1u << DOUBLE_EXPONENT_BITS) - 1));
  // Case distinction; exit early for the easy cases.
  if (ieeeExponent == ((1u << DOUBLE_EXPONENT_BITS) - 1u) || (ieeeExponent == 0 && ieeeMantissa == 0)) {
    return copy_special_str(result, ieeeSign, ieeeExponent, ieeeMantissa);
  }

  floating_decimal_64 v;
  const bool isSmallInt = d2d_small_int(ieeeMantissa, ieeeExponent, &v);
  if (isSmallInt) {
    // For small integers in the range [1, 2^53), v.mantissa might contain trailing (decimal) zeros.
    // For scientific notation we need to move these zeros into the exponent.
    // (This is not needed for fixed-point notation, so it might be beneficial to trim
    // trailing zeros in to_chars only if needed - once fixed-point notation output is implemented.)
    for (;;) {
      const uint64_t q = div10(v.mantissa);
      const uint32_t r = ((uint32_t) v.mantissa) - 10 * ((uint32_t) q);
      if (r != 0) {
        break;
      }
      v.mantissa = q;
      ++v.exponent;
    }
  } else {
    v = d2d(ieeeMantissa, ieeeExponent);
  }

  return to_chars(v, ieeeSign, result);
}

#if 0
static void go_ryu_d2s_buffered(double f, char* result) {
  const int index = go_ryu_d2s_buffered_n(f, result);

  // Terminate the string.
  result[index] = '\0';
}
#endif

#if 0
static char* go_ryu_d2s(double f) {
  char* const result = (char*) malloc(25);
  go_ryu_d2s_buffered(f, result);
  return result;
}
#endif
// End of file d2s.c imported from ryu

#ifdef GOFFICE_WITH_LONG_DOUBLE
// File ryu_generic_128.h imported from ryu
// Copyright 2018 Ulf Adams
//
// The contents of this file may be used under the terms of the Apache License,
// Version 2.0.
//
//    (See accompanying file LICENSE-Apache or copy at
//     http://www.apache.org/licenses/LICENSE-2.0)
//
// Alternatively, the contents of this file may be used under the terms of
// the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE-Boost or copy at
//     https://www.boost.org/LICENSE_1_0.txt)
//
// Unless required by applicable law or agreed to in writing, this software
// is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.
#ifndef RYU_GENERIC_128_H
#define RYU_GENERIC_128_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// This is a generic 128-bit implementation of float to shortest conversion
// using the Ryu algorithm. It can handle any IEEE-compatible floating-point
// type up to 128 bits. In order to use this correctly, you must use the
// appropriate *_to_fd128 function for the underlying type - DO NOT CAST your
// input to another floating-point type, doing so will result in incorrect
// output!
//
// For any floating-point type that is not natively defined by the compiler,
// you can use go_ryu_generic_binary_to_decimal to work directly on the underlying bit
// representation.

#define FD128_EXCEPTIONAL_EXPONENT 0x7FFFFFFF

// A floating decimal representing (-1)^s * m * 10^e.
struct floating_decimal_128 {
  __uint128_t mantissa;
  int32_t exponent;
  bool sign;
};

#if 0
static struct floating_decimal_128 go_ryu_float_to_fd128(float f);
#endif
#if 0
static struct floating_decimal_128 go_ryu_double_to_fd128(double d);
#endif

// According to wikipedia (https://en.wikipedia.org/wiki/Long_double), this likely only works on
// x86 with specific compilers (clang?). May need an ifdef.
static struct floating_decimal_128 go_ryu_long_double_to_fd128(long double d);

// Converts the given binary floating point number to the shortest decimal floating point number
// that still accurately represents it.
static struct floating_decimal_128 go_ryu_generic_binary_to_decimal(
    const __uint128_t bits, const uint32_t mantissaBits, const uint32_t exponentBits, const bool explicitLeadingBit);

// Converts the given decimal floating point number to a string, writing to result, and returning
// the number characters written. Does not terminate the buffer with a 0. In the worst case, this
// function can write up to 53 characters.
//
// Maximal char buffer requirement:
// sign + mantissa digits + decimal dot + 'E' + exponent sign + exponent digits
// = 1 + 39 + 1 + 1 + 1 + 10 = 53
static int go_ryu_generic_to_chars(const struct floating_decimal_128 v, char* const result);

#ifdef __cplusplus
}
#endif

#endif // RYU_GENERIC_128_H
// End of file ryu_generic_128.h imported from ryu
#endif // GOFFICE_WITH_LONG_DOUBLE

#ifdef GOFFICE_WITH_LONG_DOUBLE
// File generic_128.h imported from ryu
// Copyright 2018 Ulf Adams
//
// The contents of this file may be used under the terms of the Apache License,
// Version 2.0.
//
//    (See accompanying file LICENSE-Apache or copy at
//     http://www.apache.org/licenses/LICENSE-2.0)
//
// Alternatively, the contents of this file may be used under the terms of
// the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE-Boost or copy at
//     https://www.boost.org/LICENSE_1_0.txt)
//
// Unless required by applicable law or agreed to in writing, this software
// is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.
#ifndef RYU_GENERIC128_H
#define RYU_GENERIC128_H

#include <assert.h>
#include <stdint.h>

typedef __uint128_t uint128_t;

#define FLOAT_128_POW5_INV_BITCOUNT 249
#define FLOAT_128_POW5_BITCOUNT 249
#define POW5_TABLE_SIZEl 56

// These tables are ~4.5 kByte total, compared to ~160 kByte for the full tables.

// There's no way to define 128-bit constants in C, so we use little-endian
// pairs of 64-bit constants.
static const uint64_t GENERIC_POW5_TABLE[POW5_TABLE_SIZEl][2] = {
 {                    1u,                    0u },
 {                    5u,                    0u },
 {                   25u,                    0u },
 {                  125u,                    0u },
 {                  625u,                    0u },
 {                 3125u,                    0u },
 {                15625u,                    0u },
 {                78125u,                    0u },
 {               390625u,                    0u },
 {              1953125u,                    0u },
 {              9765625u,                    0u },
 {             48828125u,                    0u },
 {            244140625u,                    0u },
 {           1220703125u,                    0u },
 {           6103515625u,                    0u },
 {          30517578125u,                    0u },
 {         152587890625u,                    0u },
 {         762939453125u,                    0u },
 {        3814697265625u,                    0u },
 {       19073486328125u,                    0u },
 {       95367431640625u,                    0u },
 {      476837158203125u,                    0u },
 {     2384185791015625u,                    0u },
 {    11920928955078125u,                    0u },
 {    59604644775390625u,                    0u },
 {   298023223876953125u,                    0u },
 {  1490116119384765625u,                    0u },
 {  7450580596923828125u,                    0u },
 {   359414837200037393u,                    2u },
 {  1797074186000186965u,                   10u },
 {  8985370930000934825u,                   50u },
 {  8033366502585570893u,                  252u },
 {  3273344365508751233u,                 1262u },
 { 16366721827543756165u,                 6310u },
 {  8046632842880574361u,                31554u },
 {  3339676066983768573u,               157772u },
 { 16698380334918842865u,               788860u },
 {  9704925379756007861u,              3944304u },
 { 11631138751360936073u,             19721522u },
 {  2815461535676025517u,             98607613u },
 { 14077307678380127585u,            493038065u },
 { 15046306170771983077u,           2465190328u },
 {  1444554559021708921u,          12325951644u },
 {  7222772795108544605u,          61629758220u },
 { 17667119901833171409u,         308148791101u },
 { 14548623214327650581u,        1540743955509u },
 { 17402883850509598057u,        7703719777548u },
 { 13227442957709783821u,       38518598887744u },
 { 10796982567420264257u,      192592994438723u },
 { 17091424689682218053u,      962964972193617u },
 { 11670147153572883801u,     4814824860968089u },
 {  3010503546735764157u,    24074124304840448u },
 { 15052517733678820785u,   120370621524202240u },
 {  1475612373555897461u,   601853107621011204u },
 {  7378061867779487305u,  3009265538105056020u },
 { 18443565265187884909u, 15046327690525280101u }
};

static const uint64_t GENERIC_POW5_SPLIT[89][4] = {
 {                    0u,                    0u,                    0u,    72057594037927936u },
 {                    0u,  5206161169240293376u,  4575641699882439235u,    73468396926392969u },
 {  3360510775605221349u,  6983200512169538081u,  4325643253124434363u,    74906821675075173u },
 { 11917660854915489451u,  9652941469841108803u,   946308467778435600u,    76373409087490117u },
 {  1994853395185689235u, 16102657350889591545u,  6847013871814915412u,    77868710555449746u },
 {   958415760277438274u, 15059347134713823592u,  7329070255463483331u,    79393288266368765u },
 {  2065144883315240188u,  7145278325844925976u, 14718454754511147343u,    80947715414629833u },
 {  8980391188862868935u, 13709057401304208685u,  8230434828742694591u,    82532576417087045u },
 {   432148644612782575u,  7960151582448466064u, 12056089168559840552u,    84148467132788711u },
 {   484109300864744403u, 15010663910730448582u, 16824949663447227068u,    85795995087002057u },
 { 14793711725276144220u, 16494403799991899904u, 10145107106505865967u,    87475779699624060u },
 { 15427548291869817042u, 12330588654550505203u, 13980791795114552342u,    89188452518064298u },
 {  9979404135116626552u, 13477446383271537499u, 14459862802511591337u,    90934657454687378u },
 { 12385121150303452775u,  9097130814231585614u,  6523855782339765207u,    92715051028904201u },
 {  1822931022538209743u, 16062974719797586441u,  3619180286173516788u,    94530302614003091u },
 { 12318611738248470829u, 13330752208259324507u, 10986694768744162601u,    96381094688813589u },
 { 13684493829640282333u,  7674802078297225834u, 15208116197624593182u,    98268123094297527u },
 {  5408877057066295332u,  6470124174091971006u, 15112713923117703147u,   100192097295163851u },
 { 11407083166564425062u, 18189998238742408185u,  4337638702446708282u,   102153740646605557u },
 {  4112405898036935485u,   924624216579956435u, 14251108172073737125u,   104153790666259019u },
 { 16996739107011444789u, 10015944118339042475u,  2395188869672266257u,   106192999311487969u },
 {  4588314690421337879u,  5339991768263654604u, 15441007590670620066u,   108272133262096356u },
 {  2286159977890359825u, 14329706763185060248u,  5980012964059367667u,   110391974208576409u },
 {  9654767503237031099u, 11293544302844823188u, 11739932712678287805u,   112553319146000238u },
 { 11362964448496095896u,  7990659682315657680u,   251480263940996374u,   114756980673665505u },
 {  1423410421096377129u, 14274395557581462179u, 16553482793602208894u,   117003787300607788u },
 {  2070444190619093137u, 11517140404712147401u, 11657844572835578076u,   119294583757094535u },
 {  7648316884775828921u, 15264332483297977688u,   247182277434709002u,   121630231312217685u },
 { 17410896758132241352u, 10923914482914417070u, 13976383996795783649u,   124011608097704390u },
 {  9542674537907272703u,  3079432708831728956u, 14235189590642919676u,   126439609438067572u },
 { 10364666969937261816u,  8464573184892924210u, 12758646866025101190u,   128915148187220428u },
 { 14720354822146013883u, 11480204489231511423u,  7449876034836187038u,   131439155071681461u },
 {  1692907053653558553u, 17835392458598425233u,  1754856712536736598u,   134012579040499057u },
 {  5620591334531458755u, 11361776175667106627u, 13350215315297937856u,   136636387622027174u },
 { 17455759733928092601u, 10362573084069962561u, 11246018728801810510u,   139311567287686283u },
 {  2465404073814044982u, 17694822665274381860u,  1509954037718722697u,   142039123822846312u },
 {  2152236053329638369u, 11202280800589637091u, 16388426812920420176u,    72410041352485523u },
 { 17319024055671609028u, 10944982848661280484u,  2457150158022562661u,    73827744744583080u },
 { 17511219308535248024u,  5122059497846768077u,  2089605804219668451u,    75273205100637900u },
 { 10082673333144031533u, 14429008783411894887u, 12842832230171903890u,    76746965869337783u },
 { 16196653406315961184u, 10260180891682904501u, 10537411930446752461u,    78249581139456266u },
 { 15084422041749743389u,   234835370106753111u, 16662517110286225617u,    79781615848172976u },
 {  8199644021067702606u,  3787318116274991885u,  7438130039325743106u,    81343645993472659u },
 { 12039493937039359765u,  9773822153580393709u,  5945428874398357806u,    82936258850702722u },
 {   984543865091303961u,  7975107621689454830u,  6556665988501773347u,    84560053193370726u },
 {  9633317878125234244u, 16099592426808915028u,  9706674539190598200u,    86215639518264828u },
 {  6860695058870476186u,  4471839111886709592u,  7828342285492709568u,    87903640274981819u },
 { 14583324717644598331u,  4496120889473451238u,  5290040788305728466u,    89624690099949049u },
 { 18093669366515003715u, 12879506572606942994u, 18005739787089675377u,    91379436055028227u },
 { 17997493966862379937u, 14646222655265145582u, 10265023312844161858u,    93168537870790806u },
 { 12283848109039722318u, 11290258077250314935u,  9878160025624946825u,    94992668194556404u },
 {  8087752761883078164u,  5262596608437575693u, 11093553063763274413u,    96852512843287537u },
 { 15027787746776840781u, 12250273651168257752u,  9290470558712181914u,    98748771061435726u },
 { 15003915578366724489u,  2937334162439764327u,  5404085603526796602u,   100682155783835929u },
 {  5225610465224746757u, 14932114897406142027u,  2774647558180708010u,   102653393903748137u },
 { 17112957703385190360u, 12069082008339002412u,  3901112447086388439u,   104663226546146909u },
 {  4062324464323300238u,  3992768146772240329u, 15757196565593695724u,   106712409346361594u },
 {  5525364615810306701u, 11855206026704935156u, 11344868740897365300u,   108801712734172003u },
 {  9274143661888462646u,  4478365862348432381u, 18010077872551661771u,   110931922223466333u },
 { 12604141221930060148u,  8930937759942591500u,  9382183116147201338u,   113103838707570263u },
 { 14513929377491886653u,  1410646149696279084u,   587092196850797612u,   115318278760358235u },
 {  2226851524999454362u,  7717102471110805679u,  7187441550995571734u,   117576074943260147u },
 {  5527526061344932763u,  2347100676188369132u, 16976241418824030445u,   119878076118278875u },
 {  6088479778147221611u, 17669593130014777580u, 10991124207197663546u,   122225147767136307u },
 { 11107734086759692041u,  3391795220306863431u, 17233960908859089158u,   124618172316667879u },
 {  7913172514655155198u, 17726879005381242552u,   641069866244011540u,   127058049470587962u },
 { 12596991768458713949u, 15714785522479904446u,  6035972567136116512u,   129545696547750811u },
 { 16901996933781815980u,  4275085211437148707u, 14091642539965169063u,   132082048827034281u },
 {  7524574627987869240u, 15661204384239316051u,  2444526454225712267u,   134668059898975949u },
 {  8199251625090479942u,  6803282222165044067u, 16064817666437851504u,   137304702024293857u },
 {  4453256673338111920u, 15269922543084434181u,  3139961729834750852u,   139992966499426682u },
 { 15841763546372731299u,  3013174075437671812u,  4383755396295695606u,   142733864029230733u },
 {  9771896230907310329u,  4900659362437687569u, 12386126719044266361u,    72764212553486967u },
 {  9420455527449565190u,  1859606122611023693u,  6555040298902684281u,    74188850200884818u },
 {  5146105983135678095u,  2287300449992174951u,  4325371679080264751u,    75641380576797959u },
 { 11019359372592553360u,  8422686425957443718u,  7175176077944048210u,    77122349788024458u },
 { 11005742969399620716u,  4132174559240043701u,  9372258443096612118u,    78632314633490790u },
 {  8887589641394725840u,  8029899502466543662u, 14582206497241572853u,    80171842813591127u },
 {   360247523705545899u, 12568341805293354211u, 14653258284762517866u,    81741513143625247u },
 { 12314272731984275834u,  4740745023227177044u,  6141631472368337539u,    83341915771415304u },
 {   441052047733984759u,  7940090120939869826u, 11750200619921094248u,    84973652399183278u },
 {  3436657868127012749u,  9187006432149937667u, 16389726097323041290u,    86637336509772529u },
 { 13490220260784534044u, 15339072891382896702u,  8846102360835316895u,    88333593597298497u },
 {  4125672032094859833u,   158347675704003277u, 10592598512749774447u,    90063061402315272u },
 { 12189928252974395775u,  2386931199439295891u,  7009030566469913276u,    91826390151586454u },
 {  9256479608339282969u,  2844900158963599229u, 11148388908923225596u,    93624242802550437u },
 { 11584393507658707408u,  2863659090805147914u,  9873421561981063551u,    95457295292572042u },
 { 13984297296943171390u,  1931468383973130608u, 12905719743235082319u,    97326236793074198u },
 {  5837045222254987499u, 10213498696735864176u, 14893951506257020749u,    99231769968645227u }
};

// Unfortunately, the results are sometimes off by one or two. We use an additional
// lookup table to store those cases and adjust the result.
static const uint64_t POW5_ERRORS[156] = {
 0x0000000000000000u, 0x0000000000000000u, 0x0000000000000000u, 0x9555596400000000u,
 0x65a6569525565555u, 0x4415551445449655u, 0x5105015504144541u, 0x65a69969a6965964u,
 0x5054955969959656u, 0x5105154515554145u, 0x4055511051591555u, 0x5500514455550115u,
 0x0041140014145515u, 0x1005440545511051u, 0x0014405450411004u, 0x0414440010500000u,
 0x0044000440010040u, 0x5551155000004001u, 0x4554555454544114u, 0x5150045544005441u,
 0x0001111400054501u, 0x6550955555554554u, 0x1504159645559559u, 0x4105055141454545u,
 0x1411541410405454u, 0x0415555044545555u, 0x0014154115405550u, 0x1540055040411445u,
 0x0000000500000000u, 0x5644000000000000u, 0x1155555591596555u, 0x0410440054569565u,
 0x5145100010010005u, 0x0555041405500150u, 0x4141450455140450u, 0x0000000144000140u,
 0x5114004001105410u, 0x4444100404005504u, 0x0414014410001015u, 0x5145055155555015u,
 0x0141041444445540u, 0x0000100451541414u, 0x4105041104155550u, 0x0500501150451145u,
 0x1001050000004114u, 0x5551504400141045u, 0x5110545410151454u, 0x0100001400004040u,
 0x5040010111040000u, 0x0140000150541100u, 0x4400140400104110u, 0x5011014405545004u,
 0x0000000044155440u, 0x0000000010000000u, 0x1100401444440001u, 0x0040401010055111u,
 0x5155155551405454u, 0x0444440015514411u, 0x0054505054014101u, 0x0451015441115511u,
 0x1541411401140551u, 0x4155104514445110u, 0x4141145450145515u, 0x5451445055155050u,
 0x4400515554110054u, 0x5111145104501151u, 0x565a655455500501u, 0x5565555555525955u,
 0x0550511500405695u, 0x4415504051054544u, 0x6555595965555554u, 0x0100915915555655u,
 0x5540001510001001u, 0x5450051414000544u, 0x1405010555555551u, 0x5555515555644155u,
 0x5555055595496555u, 0x5451045004415000u, 0x5450510144040144u, 0x5554155555556455u,
 0x5051555495415555u, 0x5555554555555545u, 0x0000000010005455u, 0x4000005000040000u,
 0x5565555555555954u, 0x5554559555555505u, 0x9645545495552555u, 0x4000400055955564u,
 0x0040000000000001u, 0x4004100100000000u, 0x5540040440000411u, 0x4565555955545644u,
 0x1140659549651556u, 0x0100000410010000u, 0x5555515400004001u, 0x5955545555155255u,
 0x5151055545505556u, 0x5051454510554515u, 0x0501500050415554u, 0x5044154005441005u,
 0x1455445450550455u, 0x0010144055144545u, 0x0000401100000004u, 0x1050145050000010u,
 0x0415004554011540u, 0x1000510100151150u, 0x0100040400001144u, 0x0000000000000000u,
 0x0550004400000100u, 0x0151145041451151u, 0x0000400400005450u, 0x0000100044010004u,
 0x0100054100050040u, 0x0504400005410010u, 0x4011410445500105u, 0x0000404000144411u,
 0x0101504404500000u, 0x0000005044400400u, 0x0000000014000100u, 0x0404440414000000u,
 0x5554100410000140u, 0x4555455544505555u, 0x5454105055455455u, 0x0115454155454015u,
 0x4404110000045100u, 0x4400001100101501u, 0x6596955956966a94u, 0x0040655955665965u,
 0x5554144400100155u, 0xa549495401011041u, 0x5596555565955555u, 0x5569965959549555u,
 0x969565a655555456u, 0x0000001000000000u, 0x0000000040000140u, 0x0000040100000000u,
 0x1415454400000000u, 0x5410415411454114u, 0x0400040104000154u, 0x0504045000000411u,
 0x0000001000000010u, 0x5554000000001040u, 0x5549155551556595u, 0x1455541055515555u,
 0x0510555454554541u, 0x9555555555540455u, 0x6455456555556465u, 0x4524565555654514u,
 0x5554655255559545u, 0x9555455441155556u, 0x0000000051515555u, 0x0010005040000550u,
 0x5044044040000000u, 0x1045040440010500u, 0x0000400000040000u, 0x0000000000000000u
};

static const uint64_t GENERIC_POW5_INV_SPLIT[89][4] = {
 {                    0u,                    0u,                    0u,   144115188075855872u },
 {  1573859546583440065u,  2691002611772552616u,  6763753280790178510u,   141347765182270746u },
 { 12960290449513840412u, 12345512957918226762u, 18057899791198622765u,   138633484706040742u },
 {  7615871757716765416u,  9507132263365501332u,  4879801712092008245u,   135971326161092377u },
 {  7869961150745287587u,  5804035291554591636u,  8883897266325833928u,   133360288657597085u },
 {  2942118023529634767u, 15128191429820565086u, 10638459445243230718u,   130799390525667397u },
 { 14188759758411913794u,  5362791266439207815u,  8068821289119264054u,   128287668946279217u },
 {  7183196927902545212u,  1952291723540117099u, 12075928209936341512u,   125824179589281448u },
 {  5672588001402349748u, 17892323620748423487u,  9874578446960390364u,   123407996258356868u },
 {  4442590541217566325u,  4558254706293456445u, 10343828952663182727u,   121038210542800766u },
 {  3005560928406962566u,  2082271027139057888u, 13961184524927245081u,   118713931475986426u },
 { 13299058168408384786u, 17834349496131278595u,  9029906103900731664u,   116434285200389047u },
 {  5414878118283973035u, 13079825470227392078u, 17897304791683760280u,   114198414639042157u },
 { 14609755883382484834u, 14991702445765844156u,  3269802549772755411u,   112005479173303009u },
 { 15967774957605076027u,  2511532636717499923u, 16221038267832563171u,   109854654326805788u },
 {  9269330061621627145u,  3332501053426257392u, 16223281189403734630u,   107745131455483836u },
 { 16739559299223642282u,  1873986623300664530u,  6546709159471442872u,   105676117443544318u },
 { 17116435360051202055u,  1359075105581853924u,  2038341371621886470u,   103646834405281051u },
 { 17144715798009627550u,  3201623802661132408u,  9757551605154622431u,   101656519392613377u },
 { 17580479792687825857u,  6546633380567327312u, 15099972427870912398u,    99704424108241124u },
 {  9726477118325522902u, 14578369026754005435u, 11728055595254428803u,    97789814624307808u },
 {   134593949518343635u,  5715151379816901985u,  1660163707976377376u,    95911971106466306u },
 {  5515914027713859358u,  7124354893273815720u,  5548463282858794077u,    94070187543243255u },
 {  6188403395862945512u,  5681264392632320838u, 15417410852121406654u,    92263771480600430u },
 { 15908890877468271457u, 10398888261125597540u,  4817794962769172309u,    90492043761593298u },
 {  1413077535082201005u, 12675058125384151580u,  7731426132303759597u,    88754338271028867u },
 {  1486733163972670293u, 11369385300195092554u, 11610016711694864110u,    87050001685026843u },
 {  8788596583757589684u,  3978580923851924802u,  9255162428306775812u,    85378393225389919u },
 {  7203518319660962120u, 15044736224407683725u,  2488132019818199792u,    83738884418690858u },
 {  4004175967662388707u, 18236988667757575407u, 15613100370957482671u,    82130858859985791u },
 { 18371903370586036463u,    53497579022921640u, 16465963977267203307u,    80553711981064899u },
 { 10170778323887491315u,  1999668801648976001u, 10209763593579456445u,    79006850823153334u },
 { 17108131712433974546u, 16825784443029944237u,  2078700786753338945u,    77489693813976938u },
 { 17221789422665858532u, 12145427517550446164u,  5391414622238668005u,    76001670549108934u },
 {  4859588996898795878u,  1715798948121313204u,  3950858167455137171u,    74542221577515387u },
 { 13513469241795711526u,   631367850494860526u, 10517278915021816160u,    73110798191218799u },
 { 11757513142672073111u,  2581974932255022228u, 17498959383193606459u,   143413724438001539u },
 { 14524355192525042817u,  5640643347559376447u,  1309659274756813016u,   140659771648132296u },
 {  2765095348461978538u, 11021111021896007722u,  3224303603779962366u,   137958702611185230u },
 { 12373410389187981037u, 13679193545685856195u, 11644609038462631561u,   135309501808182158u },
 { 12813176257562780151u,  3754199046160268020u,  9954691079802960722u,   132711173221007413u },
 { 17557452279667723458u,  3237799193992485824u, 17893947919029030695u,   130162739957935629u },
 { 14634200999559435155u,  4123869946105211004u,  6955301747350769239u,   127663243886350468u },
 {  2185352760627740240u,  2864813346878886844u, 13049218671329690184u,   125211745272516185u },
 {  6143438674322183002u, 10464733336980678750u,  6982925169933978309u,   122807322428266620u },
 {  1099509117817174576u, 10202656147550524081u,   754997032816608484u,   120449071364478757u },
 {  2410631293559367023u, 17407273750261453804u, 15307291918933463037u,   118136105451200587u },
 { 12224968375134586697u,  1664436604907828062u, 11506086230137787358u,   115867555084305488u },
 {  3495926216898000888u, 18392536965197424288u, 10992889188570643156u,   113642567358547782u },
 {  8744506286256259680u,  3966568369496879937u, 18342264969761820037u,   111460305746896569u },
 {  7689600520560455039u,  5254331190877624630u,  9628558080573245556u,   109319949786027263u },
 { 11862637625618819436u,  3456120362318976488u, 14690471063106001082u,   107220694767852583u },
 {  5697330450030126444u, 12424082405392918899u,   358204170751754904u,   105161751436977040u },
 { 11257457505097373622u, 15373192700214208870u,   671619062372033814u,   103142345693961148u },
 { 16850355018477166700u,  1913910419361963966u,  4550257919755970531u,   101161718304283822u },
 {  9670835567561997011u, 10584031339132130638u,  3060560222974851757u,    99219124612893520u },
 {  7698686577353054710u, 11689292838639130817u, 11806331021588878241u,    97313834264240819u },
 { 12233569599615692137u,  3347791226108469959u, 10333904326094451110u,    95445130927687169u },
 { 13049400362825383933u, 17142621313007799680u,  3790542585289224168u,    93612312028186576u },
 { 12430457242474442072u,  5625077542189557960u, 14765055286236672238u,    91814688482138969u },
 {  4759444137752473128u,  2230562561567025078u,  4954443037339580076u,    90051584438315940u },
 {  7246913525170274758u,  8910297835195760709u,  4015904029508858381u,    88322337023761438u },
 { 12854430245836432067u,  8135139748065431455u, 11548083631386317976u,    86626296094571907u },
 {  4848827254502687803u,  4789491250196085625u,  3988192420450664125u,    84962823991462151u },
 {  7435538409611286684u,   904061756819742353u, 14598026519493048444u,    83331295300025028u },
 { 11042616160352530997u,  8948390828345326218u, 10052651191118271927u,    81731096615594853u },
 { 11059348291563778943u, 11696515766184685544u,  3783210511290897367u,    80161626312626082u },
 {  7020010856491885826u,  5025093219346041680u,  8960210401638911765u,    78622294318500592u },
 { 17732844474490699984u,  7820866704994446502u,  6088373186798844243u,    77112521891678506u },
 {   688278527545590501u,  3045610706602776618u,  8684243536999567610u,    75631741404109150u },
 {  2734573255120657297u,  3903146411440697663u,  9470794821691856713u,    74179396127820347u },
 { 15996457521023071259u,  4776627823451271680u, 12394856457265744744u,    72754940025605801u },
 { 13492065758834518331u,  7390517611012222399u,  1630485387832860230u,   142715675091463768u },
 { 13665021627282055864u,  9897834675523659302u, 17907668136755296849u,   139975126841173266u },
 {  9603773719399446181u, 10771916301484339398u, 10672699855989487527u,   137287204938390542u },
 {  3630218541553511265u,  8139010004241080614u,  2876479648932814543u,   134650898807055963u },
 {  8318835909686377084u,  9525369258927993371u,  2796120270400437057u,   132065217277054270u },
 { 11190003059043290163u, 12424345635599592110u, 12539346395388933763u,   129529188211565064u },
 {  8701968833973242276u,   820569587086330727u,  2315591597351480110u,   127041858141569228u },
 {  5115113890115690487u, 16906305245394587826u,  9899749468931071388u,   124602291907373862u },
 { 15543535488939245974u, 10945189844466391399u,  3553863472349432246u,   122209572307020975u },
 {  7709257252608325038u,  1191832167690640880u, 15077137020234258537u,   119862799751447719u },
 {  7541333244210021737u,  9790054727902174575u,  5160944773155322014u,   117561091926268545u },
 { 12297384708782857832u,  1281328873123467374u,  4827925254630475769u,   115303583460052092u },
 { 13243237906232367265u, 15873887428139547641u,  3607993172301799599u,   113089425598968120u },
 { 11384616453739611114u, 15184114243769211033u, 13148448124803481057u,   110917785887682141u },
 { 17727970963596660683u,  1196965221832671990u, 14537830463956404138u,   108787847856377790u },
 { 17241367586707330931u,  8880584684128262874u, 11173506540726547818u,   106698810713789254u },
 {  7184427196661305643u, 14332510582433188173u, 14230167953789677901u,   104649889046128358u }
};

static const uint64_t POW5_INV_ERRORS[154] = {
 0x1144155514145504u, 0x0000541555401141u, 0x0000000000000000u, 0x0154454000000000u,
 0x4114105515544440u, 0x0001001111500415u, 0x4041411410011000u, 0x5550114515155014u,
 0x1404100041554551u, 0x0515000450404410u, 0x5054544401140004u, 0x5155501005555105u,
 0x1144141000105515u, 0x0541500000500000u, 0x1104105540444140u, 0x4000015055514110u,
 0x0054010450004005u, 0x4155515404100005u, 0x5155145045155555u, 0x1511555515440558u,
 0x5558544555515555u, 0x0000000000000010u, 0x5004000000000050u, 0x1415510100000010u,
 0x4545555444514500u, 0x5155151555555551u, 0x1441540144044554u, 0x5150104045544400u,
 0x5450545401444040u, 0x5554455045501400u, 0x4655155555555145u, 0x1000010055455055u,
 0x1000004000055004u, 0x4455405104000005u, 0x4500114504150545u, 0x0000000014000000u,
 0x5450000000000000u, 0x5514551511445555u, 0x4111501040555451u, 0x4515445500054444u,
 0x5101500104100441u, 0x1545115155545055u, 0x0000000000000000u, 0x1554000000100000u,
 0x5555545595551555u, 0x5555051851455955u, 0x5555555555555559u, 0x0000400011001555u,
 0x0000004400040000u, 0x5455511555554554u, 0x5614555544115445u, 0x6455156145555155u,
 0x5455855455415455u, 0x5515555144555545u, 0x0114400000145155u, 0x0000051000450511u,
 0x4455154554445100u, 0x4554150141544455u, 0x65955555559a5965u, 0x5555555854559559u,
 0x9569654559616595u, 0x1040044040005565u, 0x1010010500011044u, 0x1554015545154540u,
 0x4440555401545441u, 0x1014441450550105u, 0x4545400410504145u, 0x5015111541040151u,
 0x5145051154000410u, 0x1040001044545044u, 0x4001400000151410u, 0x0540000044040000u,
 0x0510555454411544u, 0x0400054054141550u, 0x1001041145001100u, 0x0000000140000000u,
 0x0000000014100000u, 0x1544005454000140u, 0x4050055505445145u, 0x0011511104504155u,
 0x5505544415045055u, 0x1155154445515554u, 0x0000000000004555u, 0x0000000000000000u,
 0x5101010510400004u, 0x1514045044440400u, 0x5515519555515555u, 0x4554545441555545u,
 0x1551055955551515u, 0x0150000011505515u, 0x0044005040400000u, 0x0004001004010050u,
 0x0000051004450414u, 0x0114001101001144u, 0x0401000001000001u, 0x4500010001000401u,
 0x0004100000005000u, 0x0105000441101100u, 0x0455455550454540u, 0x5404050144105505u,
 0x4101510540555455u, 0x1055541411451555u, 0x5451445110115505u, 0x1154110010101545u,
 0x1145140450054055u, 0x5555565415551554u, 0x1550559555555555u, 0x5555541545045141u,
 0x4555455450500100u, 0x5510454545554555u, 0x1510140115045455u, 0x1001050040111510u,
 0x5555454555555504u, 0x9954155545515554u, 0x6596656555555555u, 0x0140410051555559u,
 0x0011104010001544u, 0x965669659a680501u, 0x5655a55955556955u, 0x4015111014404514u,
 0x1414155554505145u, 0x0540040011051404u, 0x1010000000015005u, 0x0010054050004410u,
 0x5041104014000100u, 0x4440010500100001u, 0x1155510504545554u, 0x0450151545115541u,
 0x4000100400110440u, 0x1004440010514440u, 0x0000115050450000u, 0x0545404455541500u,
 0x1051051555505101u, 0x5505144554544144u, 0x4550545555515550u, 0x0015400450045445u,
 0x4514155400554415u, 0x4555055051050151u, 0x1511441450001014u, 0x4544554510404414u,
 0x4115115545545450u, 0x5500541555551555u, 0x5550010544155015u, 0x0144414045545500u,
 0x4154050001050150u, 0x5550511111000145u, 0x1114504055000151u, 0x5104041101451040u,
 0x0010501401051441u, 0x0010501450504401u, 0x4554585440044444u, 0x5155555951450455u,
 0x0040000400105555u, 0x0000000000000001u,
};

// Returns e == 0 ? 1 : ceil(log_2(5^e)); requires 0 <= e <= 32768.
static inline uint32_t pow5bitsl(const int32_t e) {
  assert(e >= 0);
  assert(e <= 1 << 15);
  return (uint32_t) (((e * 163391164108059ull) >> 46) + 1);
}

static inline void mul_128_256_shift(
    const uint64_t* const a, const uint64_t* const b, const uint32_t shift, const uint32_t corr, uint64_t* const result) {
  assert(shift > 0);
  assert(shift < 256);
  const uint128_t b00 = ((uint128_t) a[0]) * b[0]; // 0
  const uint128_t b01 = ((uint128_t) a[0]) * b[1]; // 64
  const uint128_t b02 = ((uint128_t) a[0]) * b[2]; // 128
  const uint128_t b03 = ((uint128_t) a[0]) * b[3]; // 196
  const uint128_t b10 = ((uint128_t) a[1]) * b[0]; // 64
  const uint128_t b11 = ((uint128_t) a[1]) * b[1]; // 128
  const uint128_t b12 = ((uint128_t) a[1]) * b[2]; // 196
  const uint128_t b13 = ((uint128_t) a[1]) * b[3]; // 256

  const uint128_t s0 = b00;       // 0   x
  const uint128_t s1 = b01 + b10; // 64  x
  const uint128_t c1 = s1 < b01;  // 196 x
  const uint128_t s2 = b02 + b11; // 128 x
  const uint128_t c2 = s2 < b02;  // 256 x
  const uint128_t s3 = b03 + b12; // 196 x
  const uint128_t c3 = s3 < b03;  // 324 x

  const uint128_t p0 = s0 + (s1 << 64);                                // 0
  const uint128_t d0 = p0 < b00;                                       // 128
  const uint128_t q1 = s2 + (s1 >> 64) + (s3 << 64);                   // 128
  const uint128_t d1 = q1 < s2;                                        // 256
  const uint128_t p1 = q1 + (c1 << 64) + d0;                           // 128
  const uint128_t d2 = p1 < q1;                                        // 256
  const uint128_t p2 = b13 + (s3 >> 64) + c2 + (c3 << 64) + d1 + d2;   // 256

  if (shift < 128) {
    const uint128_t r0 = corr + ((p0 >> shift) | (p1 << (128 - shift)));
    const uint128_t r1 = ((p1 >> shift) | (p2 << (128 - shift))) + (r0 < corr);
    result[0] = (uint64_t) r0;
    result[1] = (uint64_t) (r0 >> 64);
    result[2] = (uint64_t) r1;
    result[3] = (uint64_t) (r1 >> 64);
  } else if (shift == 128) {
    const uint128_t r0 = corr + p1;
    const uint128_t r1 = p2 + (r0 < corr);
    result[0] = (uint64_t) r0;
    result[1] = (uint64_t) (r0 >> 64);
    result[2] = (uint64_t) r1;
    result[3] = (uint64_t) (r1 >> 64);
  } else {
    const uint128_t r0 = corr + ((p1 >> (shift - 128)) | (p2 << (256 - shift)));
    const uint128_t r1 = (p2 >> (shift - 128)) + (r0 < corr);
    result[0] = (uint64_t) r0;
    result[1] = (uint64_t) (r0 >> 64);
    result[2] = (uint64_t) r1;
    result[3] = (uint64_t) (r1 >> 64);
  }
}

// Computes 5^i in the form required by Ryu, and stores it in the given pointer.
static inline void generic_computePow5(const uint32_t i, uint64_t* const result) {
  const uint32_t base = i / POW5_TABLE_SIZEl;
  const uint32_t base2 = base * POW5_TABLE_SIZEl;
  const uint64_t* const mul = GENERIC_POW5_SPLIT[base];
  if (i == base2) {
    result[0] = mul[0];
    result[1] = mul[1];
    result[2] = mul[2];
    result[3] = mul[3];
  } else {
    const uint32_t offset = i - base2;
    const uint64_t* const m = GENERIC_POW5_TABLE[offset];
    const uint32_t delta = pow5bitsl(i) - pow5bitsl(base2);
    const uint32_t corr = (uint32_t) ((POW5_ERRORS[i / 32] >> (2 * (i % 32))) & 3);
    mul_128_256_shift(m, mul, delta, corr, result);
  }
}

// Computes 5^-i in the form required by Ryu, and stores it in the given pointer.
static inline void generic_computeInvPow5(const uint32_t i, uint64_t* const result) {
  const uint32_t base = (i + POW5_TABLE_SIZEl - 1) / POW5_TABLE_SIZEl;
  const uint32_t base2 = base * POW5_TABLE_SIZEl;
  const uint64_t* const mul = GENERIC_POW5_INV_SPLIT[base]; // 1/5^base2
  if (i == base2) {
    result[0] = mul[0] + 1;
    result[1] = mul[1];
    result[2] = mul[2];
    result[3] = mul[3];
  } else {
    const uint32_t offset = base2 - i;
    const uint64_t* const m = GENERIC_POW5_TABLE[offset]; // 5^offset
    const uint32_t delta = pow5bitsl(base2) - pow5bitsl(i);
    const uint32_t corr = (uint32_t) ((POW5_INV_ERRORS[i / 32] >> (2 * (i % 32))) & 3) + 1;
    mul_128_256_shift(m, mul, delta, corr, result);
  }
}

static inline uint32_t pow5Factorl(uint128_t value) {
  for (uint32_t count = 0; value > 0; ++count) {
    if (value % 5 != 0) {
      return count;
    }
    value /= 5;
  }
  return 0;
}

// Returns true if value is divisible by 5^p.
static inline bool multipleOfPowerOf5l(const uint128_t value, const uint32_t p) {
  // I tried a case distinction on p, but there was no performance difference.
  return pow5Factorl(value) >= p;
}

// Returns true if value is divisible by 2^p.
static inline bool multipleOfPowerOf2l(const uint128_t value, const uint32_t p) {
  return (value & ((((uint128_t) 1) << p) - 1)) == 0;
}

static inline uint128_t mulShift(const uint128_t m, const uint64_t* const mul, const int32_t j) {
  assert(j > 128);
  uint64_t a[2];
  a[0] = (uint64_t) m;
  a[1] = (uint64_t) (m >> 64);
  uint64_t result[4];
  mul_128_256_shift(a, mul, j, 0, result);
  return (((uint128_t) result[1]) << 64) | result[0];
}

static inline uint32_t decimalLength(const uint128_t v) {
  static uint128_t LARGEST_POW10 = (((uint128_t) 5421010862427522170ull) << 64) | 687399551400673280ull;
  uint128_t p10 = LARGEST_POW10;
  for (uint32_t i = 39; i > 0; i--) {
    if (v >= p10) {
      return i;
    }
    p10 /= 10;
  }
  return 1;
}

// Returns floor(log_10(2^e)).
static inline uint32_t log10Pow2l(const int32_t e) {
  // The first value this approximation fails for is 2^1651 which is just greater than 10^297.
  assert(e >= 0);
  assert(e <= 1 << 15);
  return (uint32_t) ((((uint64_t) e) * 169464822037455ull) >> 49);
}

// Returns floor(log_10(5^e)).
static inline uint32_t log10Pow5l(const int32_t e) {
  // The first value this approximation fails for is 5^2621 which is just greater than 10^1832.
  assert(e >= 0);
  assert(e <= 1 << 15);
  return (uint32_t) ((((uint64_t) e) * 196742565691928ull) >> 48);
}

#endif // RYU_GENERIC128_H
// End of file generic_128.h imported from ryu
#endif // GOFFICE_WITH_LONG_DOUBLE

#ifdef GOFFICE_WITH_LONG_DOUBLE
// File generic_128.c imported from ryu
// Copyright 2018 Ulf Adams
//
// The contents of this file may be used under the terms of the Apache License,
// Version 2.0.
//
//    (See accompanying file LICENSE-Apache or copy at
//     http://www.apache.org/licenses/LICENSE-2.0)
//
// Alternatively, the contents of this file may be used under the terms of
// the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE-Boost or copy at
//     https://www.boost.org/LICENSE_1_0.txt)
//
// Unless required by applicable law or agreed to in writing, this software
// is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.

// Runtime compiler options:
// -DRYU_DEBUG Generate verbose debugging output to stdout.


#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


#ifdef RYU_DEBUG
#include <inttypes.h>
#include <stdio.h>
static char* s(uint128_t v) {
  int len = decimalLength(v);
  char* b = (char*) malloc((len + 1) * sizeof(char));
  for (int i = 0; i < len; i++) {
    const uint32_t c = (uint32_t) (v % 10);
    v /= 10;
    b[len - 1 - i] = (char) ('0' + c);
  }
  b[len] = 0;
  return b;
}
#endif

#define ONE ((uint128_t) 1)

#define FLOAT_MANTISSA_BITS 23
#define FLOAT_EXPONENT_BITS 8

#if 0
static struct floating_decimal_128 go_ryu_float_to_fd128(float f) {
  uint32_t bits = 0;
  memcpy(&bits, &f, sizeof(float));
  return go_ryu_generic_binary_to_decimal(bits, FLOAT_MANTISSA_BITS, FLOAT_EXPONENT_BITS, false);
}
#endif

#define DOUBLE_MANTISSA_BITS 52
#define DOUBLE_EXPONENT_BITS 11

#if 0
static struct floating_decimal_128 go_ryu_double_to_fd128(double d) {
  uint64_t bits = 0;
  memcpy(&bits, &d, sizeof(double));
  return go_ryu_generic_binary_to_decimal(bits, DOUBLE_MANTISSA_BITS, DOUBLE_EXPONENT_BITS, false);
}
#endif

#define LONG_DOUBLE_MANTISSA_BITS 64
#define LONG_DOUBLE_EXPONENT_BITS 15

static struct floating_decimal_128 go_ryu_long_double_to_fd128(long double d) {
  uint128_t bits = 0;
  memcpy(&bits, &d, sizeof(long double));
#ifdef RYU_DEBUG
  // For some odd reason, this ends up with noise in the top 48 bits. We can
  // clear out those bits with the following line; this is not required, the
  // conversion routine should ignore those bits, but the debug output can be
  // confusing if they aren't 0s.
  bits &= (ONE << 80) - 1;
#endif
  return go_ryu_generic_binary_to_decimal(bits, LONG_DOUBLE_MANTISSA_BITS, LONG_DOUBLE_EXPONENT_BITS, true);
}

static struct floating_decimal_128 go_ryu_generic_binary_to_decimal(
    const uint128_t bits, const uint32_t mantissaBits, const uint32_t exponentBits, const bool explicitLeadingBit) {
#ifdef RYU_DEBUG
  printf("IN=");
  for (int32_t bit = 127; bit >= 0; --bit) {
    printf("%u", (uint32_t) ((bits >> bit) & 1));
  }
  printf("\n");
#endif

  const uint32_t bias = (1u << (exponentBits - 1)) - 1;
  const bool ieeeSign = ((bits >> (mantissaBits + exponentBits)) & 1) != 0;
  const uint128_t ieeeMantissa = bits & ((ONE << mantissaBits) - 1);
  const uint32_t ieeeExponent = (uint32_t) ((bits >> mantissaBits) & ((ONE << exponentBits) - 1u));

  if (ieeeExponent == 0 && ieeeMantissa == 0) {
    struct floating_decimal_128 fd;
    fd.mantissa = 0;
    fd.exponent = 0;
    fd.sign = ieeeSign;
    return fd;
  }
  if (ieeeExponent == ((1u << exponentBits) - 1u)) {
    struct floating_decimal_128 fd;
    fd.mantissa = explicitLeadingBit ? ieeeMantissa & ((ONE << (mantissaBits - 1)) - 1) : ieeeMantissa;
    fd.exponent = FD128_EXCEPTIONAL_EXPONENT;
    fd.sign = ieeeSign;
    return fd;
  }

  int32_t e2;
  uint128_t m2;
  // We subtract 2 in all cases so that the bounds computation has 2 additional bits.
  if (explicitLeadingBit) {
    // mantissaBits includes the explicit leading bit, so we need to correct for that here.
    if (ieeeExponent == 0) {
      e2 = 1 - bias - mantissaBits + 1 - 2;
    } else {
      e2 = ieeeExponent - bias - mantissaBits + 1 - 2;
    }
    m2 = ieeeMantissa;
  } else {
    if (ieeeExponent == 0) {
      e2 = 1 - bias - mantissaBits - 2;
      m2 = ieeeMantissa;
    } else {
      e2 = ieeeExponent - bias - mantissaBits - 2;
      m2 = (ONE << mantissaBits) | ieeeMantissa;
    }
  }
  const bool even = (m2 & 1) == 0;
  const bool acceptBounds = even;

#ifdef RYU_DEBUG
  printf("-> %s %s * 2^%d\n", ieeeSign ? "-" : "+", s(m2), e2 + 2);
#endif

  // Step 2: Determine the interval of legal decimal representations.
  const uint128_t mv = 4 * m2;
  // Implicit bool -> int conversion. True is 1, false is 0.
  const uint32_t mmShift =
      (ieeeMantissa != (explicitLeadingBit ? ONE << (mantissaBits - 1) : 0))
      || (ieeeExponent == 0);

  // Step 3: Convert to a decimal power base using 128-bit arithmetic.
  uint128_t vr, vp, vm;
  int32_t e10;
  bool vmIsTrailingZeros = false;
  bool vrIsTrailingZeros = false;
  if (e2 >= 0) {
    // I tried special-casing q == 0, but there was no effect on performance.
    // This expression is slightly faster than max(0, log10Pow2l(e2) - 1).
    const uint32_t q = log10Pow2l(e2) - (e2 > 3);
    e10 = q;
    const int32_t k = FLOAT_128_POW5_INV_BITCOUNT + pow5bitsl(q) - 1;
    const int32_t i = -e2 + q + k;
    uint64_t pow5[4];
    generic_computeInvPow5(q, pow5);
    vr = mulShift(4 * m2, pow5, i);
    vp = mulShift(4 * m2 + 2, pow5, i);
    vm = mulShift(4 * m2 - 1 - mmShift, pow5, i);
#ifdef RYU_DEBUG
    printf("%s * 2^%d / 10^%d\n", s(mv), e2, q);
    printf("V+=%s\nV =%s\nV-=%s\n", s(vp), s(vr), s(vm));
#endif
    // floor(log_5(2^128)) = 55, this is very conservative
    if (q <= 55) {
      // Only one of mp, mv, and mm can be a multiple of 5, if any.
      if (mv % 5 == 0) {
        vrIsTrailingZeros = multipleOfPowerOf5l(mv, q - 1);
      } else if (acceptBounds) {
        // Same as min(e2 + (~mm & 1), pow5Factorl(mm)) >= q
        // <=> e2 + (~mm & 1) >= q && pow5Factorl(mm) >= q
        // <=> true && pow5Factorl(mm) >= q, since e2 >= q.
        vmIsTrailingZeros = multipleOfPowerOf5l(mv - 1 - mmShift, q);
      } else {
        // Same as min(e2 + 1, pow5Factorl(mp)) >= q.
        vp -= multipleOfPowerOf5l(mv + 2, q);
      }
    }
  } else {
    // This expression is slightly faster than max(0, log10Pow5l(-e2) - 1).
    const uint32_t q = log10Pow5l(-e2) - (-e2 > 1);
    e10 = q + e2;
    const int32_t i = -e2 - q;
    const int32_t k = pow5bitsl(i) - FLOAT_128_POW5_BITCOUNT;
    const int32_t j = q - k;
    uint64_t pow5[4];
    generic_computePow5(i, pow5);
    vr = mulShift(4 * m2, pow5, j);
    vp = mulShift(4 * m2 + 2, pow5, j);
    vm = mulShift(4 * m2 - 1 - mmShift, pow5, j);
#ifdef RYU_DEBUG
    printf("%s * 5^%d / 10^%d\n", s(mv), -e2, q);
    printf("%d %d %d %d\n", q, i, k, j);
    printf("V+=%s\nV =%s\nV-=%s\n", s(vp), s(vr), s(vm));
#endif
    if (q <= 1) {
      // {vr,vp,vm} is trailing zeros if {mv,mp,mm} has at least q trailing 0 bits.
      // mv = 4 m2, so it always has at least two trailing 0 bits.
      vrIsTrailingZeros = true;
      if (acceptBounds) {
        // mm = mv - 1 - mmShift, so it has 1 trailing 0 bit iff mmShift == 1.
        vmIsTrailingZeros = mmShift == 1;
      } else {
        // mp = mv + 2, so it always has at least one trailing 0 bit.
        --vp;
      }
    } else if (q < 127) { // TODO(ulfjack): Use a tighter bound here.
      // We need to compute min(ntz(mv), pow5Factorl(mv) - e2) >= q-1
      // <=> ntz(mv) >= q-1  &&  pow5Factorl(mv) - e2 >= q-1
      // <=> ntz(mv) >= q-1    (e2 is negative and -e2 >= q)
      // <=> (mv & ((1 << (q-1)) - 1)) == 0
      // We also need to make sure that the left shift does not overflow.
      vrIsTrailingZeros = multipleOfPowerOf2l(mv, q - 1);
#ifdef RYU_DEBUG
      printf("vr is trailing zeros=%s\n", vrIsTrailingZeros ? "true" : "false");
#endif
    }
  }
#ifdef RYU_DEBUG
  printf("e10=%d\n", e10);
  printf("V+=%s\nV =%s\nV-=%s\n", s(vp), s(vr), s(vm));
  printf("vm is trailing zeros=%s\n", vmIsTrailingZeros ? "true" : "false");
  printf("vr is trailing zeros=%s\n", vrIsTrailingZeros ? "true" : "false");
#endif

  // Step 4: Find the shortest decimal representation in the interval of legal representations.
  uint32_t removed = 0;
  uint8_t lastRemovedDigit = 0;
  uint128_t output;

  while (vp / 10 > vm / 10) {
    vmIsTrailingZeros &= vm % 10 == 0;
    vrIsTrailingZeros &= lastRemovedDigit == 0;
    lastRemovedDigit = (uint8_t) (vr % 10);
    vr /= 10;
    vp /= 10;
    vm /= 10;
    ++removed;
  }
#ifdef RYU_DEBUG
  printf("V+=%s\nV =%s\nV-=%s\n", s(vp), s(vr), s(vm));
  printf("d-10=%s\n", vmIsTrailingZeros ? "true" : "false");
#endif
  if (vmIsTrailingZeros) {
    while (vm % 10 == 0) {
      vrIsTrailingZeros &= lastRemovedDigit == 0;
      lastRemovedDigit = (uint8_t) (vr % 10);
      vr /= 10;
      vp /= 10;
      vm /= 10;
      ++removed;
    }
  }
#ifdef RYU_DEBUG
  printf("%s %d\n", s(vr), lastRemovedDigit);
  printf("vr is trailing zeros=%s\n", vrIsTrailingZeros ? "true" : "false");
#endif
  if (vrIsTrailingZeros && (lastRemovedDigit == 5) && (vr % 2 == 0)) {
    // Round even if the exact numbers is .....50..0.
    lastRemovedDigit = 4;
  }
  // We need to take vr+1 if vr is outside bounds or we need to round up.
  output = vr +
      ((vr == vm && (!acceptBounds || !vmIsTrailingZeros)) || (lastRemovedDigit >= 5));
  const int32_t exp = e10 + removed;

#ifdef RYU_DEBUG
  printf("V+=%s\nV =%s\nV-=%s\n", s(vp), s(vr), s(vm));
  printf("O=%s\n", s(output));
  printf("EXP=%d\n", exp);
#endif

  struct floating_decimal_128 fd;
  fd.mantissa = output;
  fd.exponent = exp;
  fd.sign = ieeeSign;
  return fd;
}

static inline int copy_special_strl(char * const result, const struct floating_decimal_128 fd) {
  if (fd.mantissa) {
    if (fd.sign) result[0] = '-';
    memcpy(result + fd.sign, "nan", 3); // String changed to match musl's fmt_fp
    return fd.sign + 3;
  }
  if (fd.sign) {
    result[0] = '-';
  }
  memcpy(result + fd.sign, "inf", 3); // String changed to match musl's fmt_fp
  return fd.sign + 3;
}

static int go_ryu_generic_to_chars(const struct floating_decimal_128 v, char* const result) {
  if (v.exponent == FD128_EXCEPTIONAL_EXPONENT) {
    return copy_special_strl(result, v);
  }

  // Step 5: Print the decimal representation.
  int index = 0;
  if (v.sign) {
    result[index++] = '-';
  }

  uint128_t output = v.mantissa;
  const uint32_t olength = decimalLength(output);

#ifdef RYU_DEBUG
  printf("DIGITS=%s\n", s(v.mantissa));
  printf("OLEN=%u\n", olength);
  printf("EXP=%u\n", v.exponent + olength);
#endif

  for (uint32_t i = 0; i < olength - 1; ++i) {
    const uint32_t c = (uint32_t) (output % 10);
    output /= 10;
    result[index + olength - i] = (char) ('0' + c);
  }
  result[index] = '0' + (uint32_t) (output % 10); // output should be < 10 by now.

  // Print decimal point if needed.
  if (olength > 1) {
    result[index + 1] = '.';
    index += olength + 1;
  } else {
    ++index;
  }

  // Print the exponent.
  result[index++] = 'E';
  int32_t exp = v.exponent + olength - 1;
  if (exp < 0) {
    result[index++] = '-';
    exp = -exp;
  }

  uint32_t elength = decimalLength(exp);
  for (uint32_t i = 0; i < elength; ++i) {
    const uint32_t c = exp % 10;
    exp /= 10;
    result[index + elength - 1 - i] = (char) ('0' + c);
  }
  index += elength;
  return index;
}
// End of file generic_128.c imported from ryu
#endif // GOFFICE_WITH_LONG_DOUBLE


#ifdef GOFFICE_WITH_LONG_DOUBLE
int go_ryu_ld2s_buffered_n (long double d, char *dst) {
  struct floating_decimal_128 fd128 = go_ryu_long_double_to_fd128(d);
  return go_ryu_generic_to_chars(fd128, dst);
}
#endif
