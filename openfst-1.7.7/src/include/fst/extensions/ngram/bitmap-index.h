// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#ifndef FST_EXTENSIONS_NGRAM_BITMAP_INDEX_H_
#define FST_EXTENSIONS_NGRAM_BITMAP_INDEX_H_

#include <utility>
#include <vector>

#include <fst/compat.h>
#include <fst/types.h>

// This class is a bitstring storage class with an index that allows
// seeking to the Nth set or clear bit in time O(Log(N)) where N is
// the length of the bit vector.  In addition, it allows counting set or
// clear bits over ranges in constant time.
//
// This is accomplished by maintaining an "secondary" index of limited
// size in bits that maintains a running count of the number of bits set
// in each block of bitmap data.  A block is defined as the number of
// uint64 values that can fit in the secondary index before an overflow
// occurs.
//
// To handle overflows, a "primary" index containing a running count of
// bits set in each block is created using the type uint32.  Therefore,
// only bitstrings with < 2**32 ones are supported.
//
// For each 64 bits of input there are 16 bits of secondary index and
// 32/kSecondaryBlockSize == 32/1023 bits of primary index,
// for a 25.05% space overhead.

namespace fst {

class BitmapIndex {
 public:
  static size_t StorageSize(size_t num_bits) {
    return ((num_bits + kStorageBlockMask) >> kStorageLogBitSize);
  }

  BitmapIndex() = default;
  BitmapIndex(BitmapIndex &&) = default;
  BitmapIndex &operator=(BitmapIndex &&) = default;

  // Convenience constructor to avoid a separate BuildIndex call.
  BitmapIndex(const uint64 *bits, std::size_t num_bits) {
    BuildIndex(bits, num_bits);
  }

  bool Get(size_t index) const {
    return (bits_[index >> kStorageLogBitSize] &
            (kOne << (index & kStorageBlockMask))) != 0;
  }

  static void Set(uint64* bits, size_t index) {
    bits[index >> kStorageLogBitSize] |= (kOne << (index & kStorageBlockMask));
  }

  static void Clear(uint64* bits, size_t index) {
    bits[index >> kStorageLogBitSize] &= ~(kOne << (index & kStorageBlockMask));
  }

  size_t Bits() const { return num_bits_; }

  size_t ArraySize() const { return StorageSize(num_bits_); }

  // Number of bytes used to store the bit vector.
  size_t ArrayBytes() const {
    return ArraySize() * sizeof(bits_[0]);
  }

  // Number of bytes used to store the primary and secondary indices.
  size_t IndexBytes() const {
    return (primary_index_.size() * sizeof(primary_index_[0]) +
            secondary_index_.size() * sizeof(secondary_index_[0]));
  }

  // Returns the number of one bits in the bitmap
  size_t GetOnesCount() const {
    // Empty bitmaps still have a non-empty primary index.
    return primary_index_.back();
  }

  // Returns the number of one bits in positions 0 to limit - 1.
  // REQUIRES: limit <= Bits()
  size_t Rank1(size_t end) const;

  // Returns the number of one bits in the range start to end - 1.
  // REQUIRES: limit <= Bits()
  size_t GetOnesCountInRange(size_t start, size_t end) const {
    return Rank1(end) - Rank1(start);
  }

  // Returns the number of zero bits in positions 0 to limit - 1.
  // REQUIRES: limit <= Bits()
  size_t Rank0(size_t end) const { return end - Rank1(end); }

  // Returns the number of zero bits in the range start to end - 1.
  // REQUIRES: limit <= Bits()
  size_t GetZeroesCountInRange(size_t start, size_t end) const {
    return end - start - GetOnesCountInRange(start, end);
  }

  // Return true if any bit between begin inclusive and end exclusive
  // is set.  0 <= begin <= end <= Bits() is required.
  //
  bool TestRange(size_t start, size_t end) const {
    return Rank1(end) > Rank1(start);
  }

  // Returns the offset to the nth set bit (zero based)
  // or Bits() if index >= number of ones
  size_t Select1(size_t bit_index) const;

  // Returns the offset to the nth clear bit (zero based)
  // or Bits() if index > number of
  size_t Select0(size_t bit_index) const;

  // Returns the offset of the nth and nth+1 clear bit (zero based),
  // equivalent to two calls to Select0, but more efficient.
  std::pair<size_t, size_t> Select0s(size_t bit_index) const;

  // Rebuilds from index for the associated Bitmap, should be called
  // whenever changes have been made to the Bitmap or else behavior
  // of the indexed bitmap methods will be undefined.
  void BuildIndex(const uint64* bits, size_t num_bits);

  // the secondary index accumulates counts until it can possibly overflow
  // this constant computes the number of uint64 units that can fit into
  // units the size of uint16.
  static const uint64 kOne = 1;
  static const uint32 kStorageBitSize = 64;
  static const uint32 kStorageLogBitSize = 6;
  // Number of secondary index entries per primary index entry.
  static const uint32 kSecondaryBlockSize =
      ((1 << 16) - 1) >> kStorageLogBitSize;

 private:
  static const uint32 kStorageBlockMask = kStorageBitSize - 1;

  // returns, from the index, the count of ones up to array_index
  size_t get_index_ones_count(size_t array_index) const;

  // because the indexes, both primary and secondary, contain a running
  // count of the population of one bits contained in [0,i), there is
  // no reason to have an element in the zeroth position as this value would
  // necessarily be zero.  (The bits are indexed in a zero based way.)  Thus
  // we don't store the 0th element in either index for non-empty bitmaps.
  // For empty bitmaps, the 0 is stored for the primary index, but not the
  // secondary index.  Both of the following functions, if greater than 0,
  // must be decremented by one before retrieving the value from the
  // corresponding array.
  // returns the 1 + the block that contains the bitindex in question
  // the inverted version works the same but looks for zeros using an inverted
  // view of the index
  size_t find_primary_block(size_t bit_index) const;

  size_t find_inverted_primary_block(size_t bit_index) const;

  // similarly, the secondary index (which resets its count to zero at
  // the end of every kSecondaryBlockSize entries) does not store the element
  // at 0.  Note that the rem_bit_index parameter is the number of bits
  // within the secondary block, after the bits accounted for by the primary
  // block have been removed (i.e. the remaining bits)  And, because we
  // reset to zero with each new block, there is no need to store those
  // actual zeros.
  // returns 1 + the secondary block that contains the bitindex in question
  size_t find_secondary_block(size_t block, size_t rem_bit_index) const;

  size_t find_inverted_secondary_block(size_t block,
                                       size_t rem_bit_index) const;

  // We create a primary index based upon the number of secondary index
  // blocks.  The primary index uses fields wide enough to accomodate any
  // index of the bitarray so cannot overflow
  // The primary index is the actual running
  // count of one bits set for all blocks (and, thus, all uint64s).
  size_t primary_index_size() const {
    // Special-case empty bitmaps to still store the 0 in the primary index.
    if (ArraySize() == 0) return 1;
    return (ArraySize() + kSecondaryBlockSize - 1) / kSecondaryBlockSize;
  }

  const uint64* bits_ = nullptr;
  size_t num_bits_ = 0;

  // The primary index contains the running popcount of all blocks
  // which means the nth value contains the popcounts of uint64 elements
  // [0,n*kSecondaryBlockSize], however, the 0th element is omitted for
  // non-empty bitmaps.
  std::vector<uint32> primary_index_;
  // The secondary index contains the running popcount of the associated
  // bitmap.  It is the same length (in units of uint16) as the
  // bitmap's map is in units of uint64s, namely ArraySize() ==
  // StorageSize(num_bits_).
  std::vector<uint16> secondary_index_;
};

}  // end namespace fst

#endif  // FST_EXTENSIONS_NGRAM_BITMAP_INDEX_H_
