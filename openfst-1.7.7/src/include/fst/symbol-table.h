// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Classes to provide symbol-to-integer and integer-to-symbol mappings.

#ifndef FST_SYMBOL_TABLE_H_
#define FST_SYMBOL_TABLE_H_

#include <functional>
#include <ios>
#include <iostream>
#include <iterator>
#include <memory>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <fst/compat.h>
#include <fst/flags.h>
#include <fst/types.h>
#include <fst/log.h>
#include <fstream>
#include <map>
#include <functional>

#ifndef OPENFST_HAVE_STD_STRING_VIEW
#ifdef __has_include
#if __has_include(<string_view>) && __cplusplus >= 201703L
#define OPENFST_HAVE_STD_STRING_VIEW 1
#endif
#endif
#endif
#ifdef OPENFST_HAVE_STD_STRING_VIEW
#include <string_view>
#else
#include <string>
#endif


DECLARE_bool(fst_compat_symbols);

namespace fst {

constexpr int64 kNoSymbol = -1;

class SymbolTable;

// WARNING: Reading via symbol table read options should
//          not be used. This is a temporary work around for
//          reading symbol ranges of previously stored symbol sets.
struct SymbolTableReadOptions {
  SymbolTableReadOptions() {}

  SymbolTableReadOptions(
      std::vector<std::pair<int64, int64>> string_hash_ranges,
      const std::string &source)
      : string_hash_ranges(std::move(string_hash_ranges)), source(source) {}

  std::vector<std::pair<int64, int64>> string_hash_ranges;
  std::string source;
};

struct SymbolTableTextOptions {
  explicit SymbolTableTextOptions(bool allow_negative_labels = false);

  bool allow_negative_labels;
  std::string fst_field_separator;
};

namespace internal {

extern const int kLineLen;

// List of symbols with a dense hash for looking up symbol index, rehashing at
// 75% occupancy.
class DenseSymbolMap {
 public:
  // Argument type for symbol lookups and inserts.

#ifdef OPENFST_HAVE_STD_STRING_VIEW
  using KeyType = std::string_view;
#else
  using KeyType = const std::string &;
#endif  // OPENFST_HAVE_STD_STRING_VIEW

  DenseSymbolMap();

  std::pair<int64, bool> InsertOrFind(KeyType key);

  int64 Find(KeyType key) const;

  size_t Size() const { return symbols_.size(); }

  const std::string &GetSymbol(size_t idx) const { return symbols_[idx]; }

  void RemoveSymbol(size_t idx);

  void ShrinkToFit();

 private:
  static constexpr int64 kEmptyBucket = -1;

  // num_buckets must be power of 2.
  void Rehash(size_t num_buckets);

  size_t GetHash(KeyType key) const { return str_hash_(key) & hash_mask_; }

  const std::hash<typename std::remove_const<
      typename std::remove_reference<KeyType>::type>::type>
      str_hash_;
  std::vector<std::string> symbols_;
  std::vector<int64> buckets_;
  uint64 hash_mask_;
};

// Base class for SymbolTable implementations.
// Use either MutableSymbolTableImpl or ConstSymbolTableImpl to derive
// implementation classes.
class SymbolTableImplBase {
 public:
  using SymbolType = DenseSymbolMap::KeyType;

  SymbolTableImplBase() = default;
  virtual ~SymbolTableImplBase() = default;

  // Enforce copying through Copy().
  SymbolTableImplBase(const SymbolTableImplBase &) = delete;
  SymbolTableImplBase &operator=(const SymbolTableImplBase &) = delete;

  virtual std::unique_ptr<SymbolTableImplBase> Copy() const = 0;

  virtual bool Write(std::ostream &strm) const = 0;

  virtual int64 AddSymbol(SymbolType symbol, int64 key) = 0;
  virtual int64 AddSymbol(SymbolType symbol) = 0;

  virtual void RemoveSymbol(int64 key) = 0;

  virtual std::string Find(int64 key) const = 0;
  virtual int64 Find(SymbolType symbol) const = 0;

  virtual bool Member(int64 key) const { return !Find(key).empty(); }
  virtual bool Member(SymbolType symbol) const {
    return Find(symbol) != kNoSymbol;
  }

  virtual void AddTable(const SymbolTable &table) = 0;

  virtual int64 GetNthKey(ssize_t pos) const = 0;

  virtual const std::string &Name() const = 0;
  virtual void SetName(const std::string &new_name) = 0;

  virtual const std::string &CheckSum() const = 0;
  virtual const std::string &LabeledCheckSum() const = 0;

  virtual int64 AvailableKey() const = 0;
  virtual size_t NumSymbols() const = 0;

  virtual bool IsMutable() const = 0;
};

// Base class for SymbolTable implementations supporting Add/Remove.
class MutableSymbolTableImpl : public SymbolTableImplBase {
 public:
  void AddTable(const SymbolTable &table) override;
  bool IsMutable() const final { return true; }
};

// Base class for immutable SymbolTable implementations.
class ConstSymbolTableImpl : public SymbolTableImplBase {
 public:
  std::unique_ptr<SymbolTableImplBase> Copy() const final;

  int64 AddSymbol(SymbolType symbol, int64 key) final;
  int64 AddSymbol(SymbolType symbol) final;
  void RemoveSymbol(int64 key) final;
  void SetName(const std::string &new_name) final;
  void AddTable(const SymbolTable &table) final;
  bool IsMutable() const final { return false; }
};

// Default SymbolTable implementation using DenseSymbolMap and std::map.
// Provides the common text and binary format serialization.
class SymbolTableImpl final : public MutableSymbolTableImpl {
 public:
  using SymbolType = DenseSymbolMap::KeyType;

  explicit SymbolTableImpl(const std::string &name)
      : name_(name),
        available_key_(0),
        dense_key_limit_(0),
        check_sum_finalized_(false) {}

  SymbolTableImpl(const SymbolTableImpl &impl)
      : name_(impl.name_),
        available_key_(impl.available_key_),
        dense_key_limit_(impl.dense_key_limit_),
        symbols_(impl.symbols_),
        idx_key_(impl.idx_key_),
        key_map_(impl.key_map_),
        check_sum_finalized_(false) {}

  std::unique_ptr<SymbolTableImplBase> Copy() const override {
    return std::unique_ptr<SymbolTableImplBase>(new SymbolTableImpl(*this));
  }

  int64 AddSymbol(SymbolType symbol, int64 key) override;

  int64 AddSymbol(SymbolType symbol) override {
    return AddSymbol(symbol, available_key_);
  }

  // Removes the symbol with the given key. The removal is costly
  // (O(NumSymbols)) and may reduce the efficiency of Find() because of a
  // potentially reduced size of the dense key interval.
  void RemoveSymbol(int64 key) override;

  static SymbolTableImpl *ReadText(
      std::istream &strm, const std::string &name,
      const SymbolTableTextOptions &opts = SymbolTableTextOptions());

  static SymbolTableImpl* Read(std::istream &strm,
                               const SymbolTableReadOptions &opts);

  bool Write(std::ostream &strm) const override;

  // Returns the string associated with the key. If the key is out of
  // range (<0, >max), return an empty string.
  std::string Find(int64 key) const override;

  // Returns the key associated with the symbol; if the symbol
  // does not exists, returns kNoSymbol.
  int64 Find(SymbolType symbol) const override {
    int64 idx = symbols_.Find(symbol);
    if (idx == kNoSymbol || idx < dense_key_limit_) return idx;
    return idx_key_[idx - dense_key_limit_];
  }

  int64 GetNthKey(ssize_t pos) const override {
    if (pos < 0 || pos >= symbols_.Size()) return kNoSymbol;
    if (pos < dense_key_limit_) return pos;
    return Find(symbols_.GetSymbol(pos));
  }

  const std::string &Name() const override { return name_; }

  void SetName(const std::string &new_name) override { name_ = new_name; }

  const std::string &CheckSum() const override {
    MaybeRecomputeCheckSum();
    return check_sum_string_;
  }

  const std::string &LabeledCheckSum() const override {
    MaybeRecomputeCheckSum();
    return labeled_check_sum_string_;
  }

  int64 AvailableKey() const override { return available_key_; }

  size_t NumSymbols() const override { return symbols_.Size(); }

  void ShrinkToFit();

 private:
  // Recomputes the checksums (both of them) if we've had changes since the last
  // computation (i.e., if check_sum_finalized_ is false).
  // Takes ~2.5 microseconds (dbg) or ~230 nanoseconds (opt) on a 2.67GHz Xeon
  // if the checksum is up-to-date (requiring no recomputation).
  void MaybeRecomputeCheckSum() const;

  std::string name_;
  int64 available_key_;
  int64 dense_key_limit_;

  DenseSymbolMap symbols_;
  // Maps index to key for index >= dense_key_limit:
  //   key = idx_key_[index - dense_key_limit]
  std::vector<int64> idx_key_;
  // Maps key to index for key >= dense_key_limit_.
  //  index = key_map_[key]
  std::map<int64, int64> key_map_;

  mutable bool check_sum_finalized_;
  mutable std::string check_sum_string_;
  mutable std::string labeled_check_sum_string_;
  mutable Mutex check_sum_mutex_;
};

}  // namespace internal

// Symbol (string) to integer (and reverse) mapping.
//
// The SymbolTable implements the mappings of labels to strings and reverse.
// SymbolTables are used to describe the alphabet of the input and output
// labels for arcs in a Finite State Transducer.
//
// SymbolTables are reference-counted and can therefore be shared across
// multiple machines. For example a language model grammar G, with a
// SymbolTable for the words in the language model can share this symbol
// table with the lexical representation L o G.
class SymbolTable {
 public:
  using SymbolType = internal::SymbolTableImpl::SymbolType;

  class iterator {
   public:
    // TODO(wolfsonkin): Expand `SymbolTable::iterator` to be a random access
    // iterator.
    using iterator_category = std::input_iterator_tag;

    class value_type {
     public:
      // Return the label of the current symbol.
      int64 Label() const { return key_; }

      // Return the string of the current symbol.
      // TODO(wolfsonkin): Consider adding caching.
      std::string Symbol() const { return table_->Find(key_); }

     private:
      explicit value_type(const SymbolTable &table, ssize_t pos)
          : table_(&table), key_(table.GetNthKey(pos)) {}

      // Sets this item to the pos'th element in the symbol table
      void SetPosition(ssize_t pos) { key_ = table_->GetNthKey(pos); }

      friend class SymbolTable::iterator;

      const SymbolTable *table_;  // Does not own the underlying SymbolTable.
      int64 key_;
    };

    using difference_type = std::ptrdiff_t;
    using pointer = const value_type *const;
    using reference = const value_type &;

    iterator &operator++() {
      ++pos_;
      if (pos_ < nsymbols_) iter_item_.SetPosition(pos_);
      return *this;
    }

    iterator operator++(int) {
      iterator retval = *this;
      ++(*this);
      return retval;
    }

    bool operator==(const iterator &that) const { return (pos_ == that.pos_); }

    bool operator!=(const iterator &that) const { return !(*this == that); }

    reference operator*() { return iter_item_; }

    pointer operator->() const { return &iter_item_; }

   private:
    explicit iterator(const SymbolTable &table, ssize_t pos = 0)
        : pos_(pos), nsymbols_(table.NumSymbols()), iter_item_(table, pos) {}

    friend class SymbolTable;

    ssize_t pos_;
    size_t nsymbols_;
    value_type iter_item_;
  };

  using const_iterator = iterator;

  // Constructs symbol table with an optional name.
  explicit SymbolTable(const std::string &name = "<unspecified>")
      : impl_(std::make_shared<internal::SymbolTableImpl>(name)) {}

  virtual ~SymbolTable() {}

  // Reads a text representation of the symbol table from an istream. Pass a
  // name to give the resulting SymbolTable.
  static SymbolTable *ReadText(
      std::istream &strm, const std::string &name,
      const SymbolTableTextOptions &opts = SymbolTableTextOptions()) {
    std::shared_ptr<internal::SymbolTableImpl> impl(
        internal::SymbolTableImpl::ReadText(strm, name, opts));
    return impl ? new SymbolTable(impl) : nullptr;
  }

  // Reads a text representation of the symbol table.
  static SymbolTable *ReadText(
      const std::string &source,
      const SymbolTableTextOptions &opts = SymbolTableTextOptions());

  // WARNING: Reading via symbol table read options should not be used. This is
  // a temporary work-around.
  static SymbolTable* Read(std::istream &strm,
                           const SymbolTableReadOptions &opts) {
    std::shared_ptr<internal::SymbolTableImpl> impl(
        internal::SymbolTableImpl::Read(strm, opts));
    return impl ? new SymbolTable(impl) : nullptr;
  }

  // Reads a binary dump of the symbol table from a stream.
  static SymbolTable *Read(std::istream &strm, const std::string &source) {
    SymbolTableReadOptions opts;
    opts.source = source;
    return Read(strm, opts);
  }

  // Reads a binary dump of the symbol table.
  static SymbolTable *Read(const std::string &source) {
    std::ifstream strm(source, std::ios_base::in | std::ios_base::binary);
    if (!strm.good()) {
      LOG(ERROR) << "SymbolTable::Read: Can't open file: " << source;
      return nullptr;
    }
    return Read(strm, source);
  }

  // Creates a reference counted copy.
  virtual SymbolTable *Copy() const { return new SymbolTable(*this); }

  // Adds another symbol table to this table. All keys will be offset by the
  // current available key (highest key in the symbol table). Note string
  // symbols with the same key will still have the same key after the symbol
  // table has been merged, but a different value. Adding symbol tables do not
  // result in changes in the base table.
  void AddTable(const SymbolTable &table) {
    MutateCheck();
    impl_->AddTable(table);
  }

  // Adds a symbol with given key to table. A symbol table also keeps track of
  // the last available key (highest key value in the symbol table).
  int64 AddSymbol(SymbolType symbol, int64 key) {
    MutateCheck();
    return impl_->AddSymbol(symbol, key);
  }

  // Adds a symbol to the table. The associated value key is automatically
  // assigned by the symbol table.
  int64 AddSymbol(SymbolType symbol) {
    MutateCheck();
    return impl_->AddSymbol(symbol);
  }

  // Returns the current available key (i.e., highest key + 1) in the symbol
  // table.
  int64 AvailableKey() const { return impl_->AvailableKey(); }

  // Return the label-agnostic MD5 check-sum for this table. All new symbols
  // added to the table will result in an updated checksum. Deprecated.
  const std::string &CheckSum() const { return impl_->CheckSum(); }

  int64 GetNthKey(ssize_t pos) const { return impl_->GetNthKey(pos); }

  // Returns the string associated with the key; if the key is out of
  // range (<0, >max), returns an empty string.
  std::string Find(int64 key) const { return impl_->Find(key); }

  // Returns the key associated with the symbol; if the symbol does not exist,
  // kNoSymbol is returned.
  int64 Find(SymbolType symbol) const { return impl_->Find(symbol); }

  // Same as CheckSum(), but returns an label-dependent version.
  const std::string &LabeledCheckSum() const {
    return impl_->LabeledCheckSum();
  }

  bool Member(int64 key) const { return impl_->Member(key); }

  bool Member(SymbolType symbol) const { return impl_->Member(symbol); }

  // Returns the name of the symbol table.
  const std::string &Name() const { return impl_->Name(); }

  // Returns the current number of symbols in table (not necessarily equal to
  // AvailableKey()).
  size_t NumSymbols() const { return impl_->NumSymbols(); }

  void RemoveSymbol(int64 key) {
    MutateCheck();
    return impl_->RemoveSymbol(key);
  }

  // Sets the name of the symbol table.
  void SetName(const std::string &new_name) {
    MutateCheck();
    impl_->SetName(new_name);
  }

  bool Write(std::ostream &strm) const { return impl_->Write(strm); }

  bool Write(const std::string &source) const;

  // Dumps a text representation of the symbol table via a stream.
  bool WriteText(std::ostream &strm, const SymbolTableTextOptions &opts =
                                         SymbolTableTextOptions()) const;

  // Dumps a text representation of the symbol table.
  bool WriteText(const std::string &source) const;

  const_iterator begin() const { return const_iterator(*this, 0); }

  const_iterator end() const { return const_iterator(*this, NumSymbols()); }

  const_iterator cbegin() const { return begin(); }

  const_iterator cend() const { return end(); }

 protected:
  explicit SymbolTable(std::shared_ptr<internal::SymbolTableImplBase> impl)
      : impl_(impl) {}

  template <class T = internal::SymbolTableImplBase>
  const T *Impl() const {
    return static_cast<const T *>(impl_.get());
  }

  template <class T = internal::SymbolTableImplBase>
  T *MutableImpl() {
    MutateCheck();
    return static_cast<T *>(impl_.get());
  }

 private:
  void MutateCheck() {
    if (impl_.unique() || !impl_->IsMutable()) return;
    std::unique_ptr<internal::SymbolTableImplBase> copy = impl_->Copy();
    CHECK(copy != nullptr);
    impl_ = std::move(copy);
  }

  std::shared_ptr<internal::SymbolTableImplBase> impl_;
};

// Iterator class for symbols in a symbol table.
class OPENFST_DEPRECATED(
    "Use SymbolTable::iterator, a C++ compliant iterator, instead")
    SymbolTableIterator {
 public:
  explicit SymbolTableIterator(const SymbolTable &table)
      : table_(table), iter_(table.begin()), end_(table.end()) {}

  ~SymbolTableIterator() {}

  // Returns whether iterator is done.
  bool Done() const { return (iter_ == end_); }

  // Return the key of the current symbol.
  int64 Value() const { return iter_->Label(); }

  // Return the string of the current symbol.
  std::string Symbol() const { return iter_->Symbol(); }

  // Advances iterator.
  void Next() { ++iter_; }

  // Resets iterator.
  void Reset() { iter_ = table_.begin(); }

 private:
  const SymbolTable &table_;
  SymbolTable::iterator iter_;
  const SymbolTable::iterator end_;
};

// Relabels a symbol table as specified by the input vector of pairs
// (old label, new label). The new symbol table only retains symbols
// for which a relabeling is explicitly specified.
//
// TODO(allauzen): consider adding options to allow for some form of implicit
// identity relabeling.
template <class Label>
SymbolTable *RelabelSymbolTable(const SymbolTable *table,
    const std::vector<std::pair<Label, Label>> &pairs) {
  auto *new_table = new SymbolTable(
      table->Name().empty() ? std::string()
                            : (std::string("relabeled_") + table->Name()));
  for (const auto &pair : pairs) {
    new_table->AddSymbol(table->Find(pair.first), pair.second);
  }
  return new_table;
}

// Returns true if the two symbol tables have equal checksums. Passing in
// nullptr for either table always returns true.
bool CompatSymbols(const SymbolTable *syms1, const SymbolTable *syms2,
                   bool warning = true);

// Symbol Table serialization.

void SymbolTableToString(const SymbolTable *table, std::string *result);

SymbolTable *StringToSymbolTable(const std::string &str);

}  // namespace fst

#endif  // FST_SYMBOL_TABLE_H_
