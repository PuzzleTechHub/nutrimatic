// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#ifndef FST_EXTENSIONS_NGRAM_NTHBIT_H_
#define FST_EXTENSIONS_NGRAM_NTHBIT_H_

#include <cstdint>

#include <fst/types.h>

#if defined(__BMI2__)  // Intel Bit Manipulation Instruction Set 2
// PDEP requires BMI2; this is present starting with Haswell.

#include <immintrin.h>

namespace fst {
// Returns the position (0-63) of the r-th 1 bit in v.
// 1 <= r <= CountOnes(v) <= 64.  Therefore, v must not be 0.
inline uint32 nth_bit(uint64 v, uint32 r) {
  // PDEP example from https://stackoverflow.com/a/27453505
  return __builtin_ctzll(_pdep_u64(uint64{1} << (r - 1), v));
}
}  // namespace fst

#elif SIZE_MAX == UINT32_MAX
// Detect 32-bit architectures via size_t.

namespace fst {
// Returns the position (0-63) of the r-th 1 bit in v.
// 1 <= r <= CountOnes(v) <= 64.  Therefore, v must not be 0.
uint32 nth_bit(uint64 v, uint32 r);
}  // namespace fst

#elif SIZE_MAX == UINT64_MAX
// Default 64-bit version, used by ARM64 and Intel < Haswell.

namespace fst {
namespace internal {
extern const uint64 kPrefixSumOverflow[65];
extern const uint8 kSelectInByte[2048];
}  // namespace internal

// Returns the position (0-63) of the r-th 1 bit in v.
// 1 <= r <= CountOnes(v) <= 64.  Therefore, v must not be 0.
//
// This version is based on the paper "Broadword Implementation of
// Rank/Select Queries" by Sebastiano Vigna, p. 5, Algorithm 2, with
// improvements from "Optimized Succinct Data Structures for Massive Data"
// by Gog & Petri, 2014.
inline uint32 nth_bit(const uint64 v, const uint32 r) {
  constexpr uint64 kOnesStep4 = 0x1111111111111111;
  constexpr uint64 kOnesStep8 = 0x0101010101010101;
  constexpr uint64 kMSBsStep8 = 0x80 * kOnesStep8;

  uint64 s = v;
  s = s - ((s >> 1) & (0x5 * kOnesStep4));
  s = (s & (0x3 * kOnesStep4)) + ((s >> 2) & (0x3 * kOnesStep4));
  s = (s + (s >> 4)) & (0xF * kOnesStep8);
  // s now contains the byte-wise popcounts of v.

  // byte_sums contains partial sums of the byte-wise popcounts.
  // That is, byte i contains the popcounts of bytes <= i.
  uint64 byte_sums = s * kOnesStep8;

  // kPrefixSumOverflow[r] == (0x80 - r) * kOnesStep8, so the high bit is
  // still set if byte_sums - r >= 0, or byte_sums >= r.  The first one set
  // is in the byte with the sum larger than or equal to r (since r is 1-based),
  // so this is the byte we need.
  const uint64 b = (byte_sums + internal::kPrefixSumOverflow[r]) & kMSBsStep8;
  // The first bit set is the high bit in the byte, so
  // num_trailing_zeros == 8 * byte_nr + 7 and the byte number is the
  // number of trailing zeros divided by 8.
  const int byte_nr = __builtin_ctzll(b) >> 3;
  const int shift = byte_nr << 3;
  // The top byte contains the whole-word popcount; we never need that.
  byte_sums <<= 8;
  // Paper uses reinterpret_cast<uint8 *>; use shift/mask instead.
  // Adjust for fact that r is 1-based.
  const int rank_in_byte = r - 1 - (byte_sums >> shift) & 0xFF;
  return shift +
         internal::kSelectInByte[(rank_in_byte << 8) + ((v >> shift) & 0xFF)];
}
}  // namespace fst

#else

#error Unrecognized architecture size

#endif

#endif  // FST_EXTENSIONS_NGRAM_NTHBIT_H_
