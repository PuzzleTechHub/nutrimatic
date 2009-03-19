
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

#include <fst/symbol-table.h>
#include <fst/util.h>

DEFINE_bool(fst_compat_symbols, true,
            "Require symbol tables to match when appropriate");

namespace fst {

// Maximum line length in textual symbols file.
const int kLineLen = 8096;

// Identifies stream data as a symbol table (and its endianity)
static const int32 kSymbolTableMagicNumber = 2125658996;

SymbolTableImpl* SymbolTableImpl::ReadText(const string &filename,
                                           bool allow_negative) {
  ifstream strm(filename.c_str());
  if (!strm) {
    LOG(ERROR) << "SymbolTable::ReadText: Can't open symbol file: "
               << filename;
    return 0;
  }

  SymbolTableImpl* impl = new SymbolTableImpl(filename);

  int64 nline = 0;
  char line[kLineLen];
  while (strm.getline(line, kLineLen)) {
    ++nline;
    vector<char *> col;
    SplitToVector(line, "\n\t ", &col, true);
    if (col.size() == 0)  // empty line
      continue;
    if (col.size() != 2) {
      LOG(ERROR) << "SymbolTable::ReadText: Bad number of columns (skipping), "
                 << "file = " << filename << ", line = " << nline;
      continue;
    }
    const char *symbol = col[0];
    const char *value = col[1];
    char *p;
    int64 key = strtoll(value, &p, 10);
    if (p < value + strlen(value) ||
        (!allow_negative && key < 0) || key == -1) {
      LOG(ERROR) << "SymbolTable::ReadText: Bad non-negative integer \""
                 << value << "\" (skipping), "
                 << "file = " << filename << ", line = " << nline;
      continue;
    }
    impl->AddSymbol(symbol, key);
  }

  return impl;
}

void SymbolTableImpl::RecomputeCheckSum() const {
  check_sum_.Reset();
  for (size_t i = 0; i < symbols_.size(); ++i) {
    check_sum_.Update(symbols_[i], strlen(symbols_[i])+1);
  }
  check_sum_finalized_ = true;
}

int64 SymbolTableImpl::AddSymbol(const string& symbol, int64 key) {
  map<const char *, int64>::const_iterator it =
      symbol_map_.find(symbol.c_str());
  if (it == symbol_map_.end()) {  // only add if not in table
    check_sum_finalized_ = false;

    char *csymbol = new char[symbol.size() + 1];
    strcpy(csymbol, symbol.c_str());
    symbols_.push_back(csymbol);
    key_map_[key] = csymbol;
    symbol_map_[csymbol] = key;

    if (key >= available_key_) {
      available_key_ = key + 1;
    }
  } else {
    // Error if symbol already in table with different key
    if (it->second != key) {
      LOG(ERROR) << "SymbolTable::AddSymbol: symbol = " << symbol
                 << " already in symbol_map_ with key = "
                 << it->second
                 << " but supplied new key = " << key
                 << " (ignoring new key)";
    }
  }
  return key;
}

static bool IsInRange(const vector<pair<int64, int64> >& ranges,
                      int64 key) {
  if (ranges.size() == 0) return true;
  for (size_t i = 0; i < ranges.size(); ++i) {
    if (key >= ranges[i].first && key <= ranges[i].second)
      return true;
  }
  return false;
}

SymbolTableImpl* SymbolTableImpl::Read(istream &strm,
                                       const SymbolTableReadOptions& opts) {
  int32 magic_number = 0;
  ReadType(strm, &magic_number);
  if (!strm) {
    LOG(ERROR) << "SymbolTable::Read: read failed";
    return 0;
  }
  string name;
  ReadType(strm, &name);
  SymbolTableImpl* impl = new SymbolTableImpl(name);
  ReadType(strm, &impl->available_key_);
  int64 size;
  ReadType(strm, &size);
  if (!strm)
    LOG(ERROR) << "SymbolTable::Read: read failed";

  string symbol;
  int64 key;
  impl->check_sum_finalized_ = false;
  for (size_t i = 0; i < size; ++i) {
    ReadType(strm, &symbol);
    ReadType(strm, &key);
    if (!strm)
      LOG(ERROR) << "SymbolTable::Read: read failed";

    char *csymbol = new char[symbol.size() + 1];
    strcpy(csymbol, symbol.c_str());
    impl->symbols_.push_back(csymbol);
    if (key == impl->dense_key_limit_ &&
        key == impl->symbols_.size() - 1)
      impl->dense_key_limit_ = impl->symbols_.size();
    else
      impl->key_map_[key] = csymbol;

    if (IsInRange(opts.string_hash_ranges, key)) {
      impl->symbol_map_[csymbol] = key;
    }
  }
  return impl;
}

bool SymbolTableImpl::Write(ostream &strm) const {
  WriteType(strm, kSymbolTableMagicNumber);
  WriteType(strm, name_);
  WriteType(strm, available_key_);
  int64 size = symbols_.size();
  WriteType(strm, size);
  // first write out dense keys
  for (int64 i = 0; i < dense_key_limit_; ++i) {
    WriteType(strm, string(symbols_[i]));
    WriteType(strm, i);
  }
  // next write out the remaining non densely packed keys
  for (map<int64, const char*>::const_iterator it = key_map_.begin();
       it != key_map_.end(); ++it) {
    WriteType(strm, string(it->second));
    WriteType(strm, it->first);
  }
  strm.flush();
  if (!strm) {
    LOG(ERROR) << "SymbolTable::Write: write failed";
    return false;
  }
  return true;
}

bool SymbolTableImpl::WriteText(ostream &strm) const {
  for (size_t i = 0; i < symbols_.size(); ++i) {
    char line[kLineLen];
    snprintf(line, kLineLen, "%s\t%lld\n", symbols_[i], Find(symbols_[i]));
    strm.write(line, strlen(line));
  }

  return true;
}

}  // namespace fst
