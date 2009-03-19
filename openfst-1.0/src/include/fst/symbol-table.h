
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// All Rights Reserved.
//
// Author : Johan Schalkwyk
//
// \file
// Classes to provide symbol-to-integer and integer-to-symbol mappings.

#ifndef FST_LIB_SYMBOL_TABLE_H__
#define FST_LIB_SYMBOL_TABLE_H__

#include <map>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

#include "fst/compat.h"
#include <iostream>
#include <fstream>

DECLARE_bool(fst_compat_symbols);

namespace fst {

// WARNING: Reading via symbol table read options should
//          not be used. This is a temporary work around for
//          reading symbol ranges of previously stored symbol sets.
struct SymbolTableReadOptions {
  SymbolTableReadOptions() { }

  SymbolTableReadOptions(vector<pair<int64, int64> > string_hash_ranges_,
                         const string& source_)
      : string_hash_ranges(string_hash_ranges_),
        source(source_) { }

  vector<pair<int64, int64> > string_hash_ranges;
  string source;
};

class SymbolTableImpl {
  friend class SymbolTableIterator;
 public:
  SymbolTableImpl(const string &name)
      : name_(name),
        available_key_(0),
        dense_key_limit_(0),
        check_sum_finalized_(false) {}

  explicit SymbolTableImpl(const SymbolTableImpl& impl)
      : name_(impl.name_),
        available_key_(0),
        dense_key_limit_(0),
        check_sum_finalized_(false) {
    for (size_t i = 0; i < impl.symbols_.size(); ++i) {
      AddSymbol(impl.symbols_[i], impl.Find(impl.symbols_[i]));
    }
  }

  ~SymbolTableImpl() {
    for (size_t i = 0; i < symbols_.size(); ++i)
      delete[] symbols_[i];
  }

  // TODO(johans): Add flag to specify whether the symbol
  //               should be indexed as string or int or both.
  int64 AddSymbol(const string& symbol, int64 key);

  int64 AddSymbol(const string& symbol) {
    int64 key = Find(symbol);
    return (key == -1) ? AddSymbol(symbol, available_key_++) : key;
  }

  void AddTable(SymbolTableImpl* table) {
    for (size_t i = 0; i < table->symbols_.size(); ++i) {
      AddSymbol(table->symbols_[i]);
    }
  }

  static SymbolTableImpl* ReadText(const string& filename,
                                   bool allow_negative = false);

  static SymbolTableImpl* Read(istream &strm,
                               const SymbolTableReadOptions& opts);

  bool Write(ostream &strm) const;

  bool WriteText(ostream &strm) const;

  //
  // Return the string associated with the key. If the key is out of
  // range (<0, >max), return an empty string.
  string Find(int64 key) const {
    if (key >=0 && key < dense_key_limit_)
      return string(symbols_[key]);

    map<int64, const char*>::const_iterator it = key_map_.find(key);
    if (it == key_map_.end()) {
      return "";
    }
    return string(it->second);
  }

  //
  // Return the key associated with the symbol. If the symbol
  // does not exists, return -1.
  int64 Find(const string& symbol) const {
    return Find(symbol.c_str());
  }

  //
  // Return the key associated with the symbol. If the symbol
  // does not exists, return -1.
  int64 Find(const char* symbol) const {
    map<const char *, int64>::const_iterator it =
        symbol_map_.find(symbol);
    if (it == symbol_map_.end()) {
      return -1;
    }
    return it->second;
  }

  const string& Name() const { return name_; }

  int IncrRefCount() const {
    return ref_count_.Incr();
  }
  int DecrRefCount() const {
    return ref_count_.Decr();
  }
  int RefCount() const {
    return ref_count_.count();
  }

  string CheckSum() const {
    if (!check_sum_finalized_) {
      RecomputeCheckSum();
      check_sum_string_ = check_sum_.Digest();
    }
    return check_sum_string_;
  }

  int64 AvailableKey() const {
    return available_key_;
  }

  size_t NumSymbols() const {
    return symbols_.size();
  }

  // private support methods
 private:
  void RecomputeCheckSum() const;

  struct StrCmp {
    bool operator()(const char *s1, const char *s2) const {
      return strcmp(s1, s2) < 0;
    }
  };

  string name_;
  int64 available_key_;
  int64 dense_key_limit_;
  vector<const char *> symbols_;
  map<int64, const char*> key_map_;
  map<const char *, int64, StrCmp> symbol_map_;

  mutable RefCounter ref_count_;
  mutable bool check_sum_finalized_;
  mutable CheckSummer check_sum_;
  mutable string check_sum_string_;
};


class SymbolTableIterator;

//
// \class SymbolTable
// \brief Symbol (string) to int and reverse mapping
//
// The SymbolTable implements the mappings of labels to strings and reverse.
// SymbolTables are used to describe the alphabet of the input and output
// labels for arcs in a Finite State Transducer.
//
// SymbolTables are reference counted and can therefore be shared across
// multiple machines. For example a language model grammar G, with a
// SymbolTable for the words in the language model can share this symbol
// table with the lexical representation L o G.
//
class SymbolTable {
  friend class SymbolTableIterator;
 public:
  static const int64 kNoSymbol = -1;

  // Construct symbol table with a unique name.
  SymbolTable(const string& name) : impl_(new SymbolTableImpl(name)) {}

  // Create a reference counted copy.
  SymbolTable(const SymbolTable& table) : impl_(table.impl_) {
    impl_->IncrRefCount();
  }

  // Derefence implentation object. When reference count hits 0, delete
  // implementation.
  ~SymbolTable() {
    if (!impl_->DecrRefCount()) delete impl_;
  }

  // create a reference counted copy
  SymbolTable* Copy() const {
    return new SymbolTable(*this);
  }

  // Add a symbol with given key to table. A symbol table also
  // keeps track of the last available key (highest key value in
  // the symbol table).
  //
  // \param symbol string symbol to add
  // \param key associated key for string symbol
  // \return the key created by the symbol table. Symbols allready added to
  //         the symbol table will not get a different key.
  int64 AddSymbol(const string& symbol, int64 key) {
    MutateCheck();
    return impl_->AddSymbol(symbol, key);
  }

  // Add a symbol to the table. The associated value key is automatically
  // assigned by the symbol table.
  //
  // \param symbol string to add to the table
  // \return the value key assigned to the associated string symbol
  int64 AddSymbol(const string& symbol) {
    MutateCheck();
    return impl_->AddSymbol(symbol);
  }

  // Add another symbol table to this table. All key values will be offset
  // by the current available key (highest key value in the symbol table).
  // Note string symbols with the same key value with still have the same
  // key value after the symbol table has been merged, but a different
  // value. Adding symbol tables do not result in changes in the base table.
  //
  // Merging N symbol tables is often useful when combining the various
  // name spaces of transducers to a unified representation.
  //
  // \param table the symbol table to add to this table
  void AddTable(const SymbolTable& table) {
    MutateCheck();
    return impl_->AddTable(table.impl_);
  }

  // return the name of the symbol table
  const string& Name() const {
    return impl_->Name();
  }

  // return the MD5 check-sum for this table. All new symbols added to
  // the table will result in an updated checksum.
  string CheckSum() const {
    return impl_->CheckSum();
  }

  // read an ascii representation of the symbol table
  static SymbolTable* ReadText(const string& filename,
                               bool allow_negative = false) {
    SymbolTableImpl* impl =
        SymbolTableImpl::ReadText(filename, allow_negative);
    if (!impl)
      return 0;
    else
      return new SymbolTable(impl);
  }

  // WARNING: Reading via symbol table read options should
  //          not be used. This is a temporary work around.
  static SymbolTable* Read(istream &strm,
                           const SymbolTableReadOptions& opts) {
    SymbolTableImpl* impl = SymbolTableImpl::Read(strm, opts);
    if (!impl)
      return 0;
    else
      return new SymbolTable(impl);
  }

  // read a binary dump of the symbol table from a stream
  static SymbolTable* Read(istream &strm, const string& source) {
    SymbolTableReadOptions opts;
    opts.source = source;
    return Read(strm, opts);
  }

  // read a binary dump of the symbol table
  static SymbolTable* Read(const string& filename) {
    ifstream strm(filename.c_str());
    if (!strm) {
      LOG(ERROR) << "SymbolTable::Read: Can't open file " << filename;
      return 0;
    }
    return Read(strm, filename);
  }

  bool Write(ostream &strm) const {
    return impl_->Write(strm);
  }

  bool Write(const string& filename) const {
    ofstream strm(filename.c_str());
    if (!strm) {
      LOG(ERROR) << "SymbolTable::Write: Can't open file " << filename;
      return false;
    }
    return Write(strm);
  }

  // Dump an ascii text representation of the symbol table via a stream
  bool WriteText(ostream &strm) const {
    return impl_->WriteText(strm);
  }

  // Dump an ascii text representation of the symbol table
  bool WriteText(const string& filename) const {
    ofstream strm(filename.c_str());
    if (!strm) {
      LOG(ERROR) << "SymbolTable::WriteText: Can't open file " << filename;
      return false;
    }
    return WriteText(strm);
  }

  // Return the string associated with the key. If the key is out of
  // range (<0, >max), log error and return an empty string.
  string Find(int64 key) const {
    return impl_->Find(key);
  }

  // Return the key associated with the symbol. If the symbol
  // does not exists, log error and  return -1
  int64 Find(const string& symbol) const {
    return impl_->Find(symbol);
  }

  // Return the key associated with the symbol. If the symbol
  // does not exists, log error and  return -1
  int64 Find(const char* symbol) const {
    return impl_->Find(symbol);
  }

  // Return the current available key (i.e highest key number+1) in
  // the symbol table
  int64 AvailableKey(void) const {
    return impl_->AvailableKey();
  }

  // Return the current number of symbols in table (not necessarily
  // equal to AvailableKey())
  size_t NumSymbols(void) const {
    return impl_->NumSymbols();
  }

private:
  explicit SymbolTable(SymbolTableImpl* impl) : impl_(impl) {}

  void MutateCheck() {
    // Copy on write
    if (impl_->RefCount() > 1) {
      impl_->DecrRefCount();
      impl_ = new SymbolTableImpl(*impl_);
    }
  }

  const SymbolTableImpl* Impl() const {
    return impl_;
  }

 private:
  SymbolTableImpl* impl_;

  void operator=(const SymbolTable &table);  // disallow
};


//
// \class SymbolTableIterator
// \brief Iterator class for symbols in a symbol table
class SymbolTableIterator {
 public:
  // Constructor creates a refcounted copy of underlying implementation
  SymbolTableIterator(const SymbolTable& symbol_table) {
    impl_ = symbol_table.Impl();
    impl_->IncrRefCount();
    pos_ = 0;
    size_ = impl_->symbols_.size();
  }

  // decrement implementation refcount, and delete if 0
  ~SymbolTableIterator() {
    if (!impl_->DecrRefCount()) delete impl_;
  }

  // is iterator done
  bool Done(void) {
    return (pos_ == size_);
  }

  // return the Value() of the current symbol (in64 key)
  int64 Value(void) {
    return impl_->Find(impl_->symbols_[pos_]);
  }

  // return the string of the current symbol
  const char* Symbol(void) {
    return impl_->symbols_[pos_];
  }

  // advance iterator forward
  void Next(void) {
    if (Done()) return;
    ++pos_;
  }

  // reset iterator
  void Reset(void) {
    pos_ = 0;
  }

 private:
  const SymbolTableImpl* impl_;
  size_t pos_;
  size_t size_;
};


// Tests compatibilty between two sets of symbol tables
inline bool CompatSymbols(const SymbolTable *syms1,  const SymbolTable *syms2,
                          bool warning = true) {
  if (!FLAGS_fst_compat_symbols) {
    return true;
  } else if (!syms1 && !syms2) {
    return true;
  } else if (syms1 && !syms2) {
    if (warning)
      LOG(WARNING) <<
          "CompatSymbols: first symbol table present but second missing";
    return false;
  } else if (!syms1 && syms2) {
    if (warning)
      LOG(WARNING) <<
          "CompatSymbols: second symbol table present but first missing";
    return false;
  } else if (syms1->CheckSum() != syms2->CheckSum()) {
    if (warning)
      LOG(WARNING) <<
          "CompatSymbols: Symbol table check sums do not match";
    return false;
  } else {
    return true;
  }
}

}  // namespace fst

#endif  // FST_LIB_SYMBOL_TABLE_H__
