// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#ifndef FST_EXTENSIONS_FAR_COMPILE_STRINGS_H_
#define FST_EXTENSIONS_FAR_COMPILE_STRINGS_H_

#include <libgen.h>

#include <fstream>
#include <istream>
#include <string>
#include <vector>

#include <fst/extensions/far/far.h>
#include <fstream>
#include <fst/string.h>

namespace fst {

// Constructs a reader that provides FSTs from a file (stream) either on a
// line-by-line basis or on a per-stream basis. Note that the freshly
// constructed reader is already set to the first input.
//
// Sample usage:
//
//   for (StringReader<Arc> reader(...); !reader.Done(); reader.Next()) {
//     auto *fst = reader.GetVectorFst();
//   }
template <class Arc>
class StringReader {
 public:
  using Label = typename Arc::Label;
  using Weight = typename Arc::Weight;

  enum EntryType { LINE = 1, FILE = 2 };

  StringReader(std::istream &istrm, const std::string &source,
               EntryType entry_type, StringTokenType token_type,
               bool allow_negative_labels, const SymbolTable *syms = nullptr,
               Label unknown_label = kNoStateId)
      : nline_(0),
        istrm_(istrm),
        source_(source),
        entry_type_(entry_type),
        token_type_(token_type),
        symbols_(syms),
        done_(false),
        compiler_(token_type, syms, unknown_label, allow_negative_labels) {
    Next();  // Initialize the reader to the first input.
  }

  bool Done() { return done_; }

  void Next() {
    VLOG(1) << "Processing source " << source_ << " at line " << nline_;
    if (!istrm_) {  // We're done if we have no more input.
      done_ = true;
      return;
    }
    if (entry_type_ == LINE) {
      getline(istrm_, content_);
      ++nline_;
    } else {
      content_.clear();
      std::string line;
      while (getline(istrm_, line)) {
        ++nline_;
        content_.append(line);
        content_.append("\n");
      }
    }
    if (!istrm_ && content_.empty())  // We're also done if we read off all the
      done_ = true;                   // whitespace at the end of a file.
  }

  VectorFst<Arc> *GetVectorFst(bool keep_symbols = false) {
    std::unique_ptr<VectorFst<Arc>> fst(new VectorFst<Arc>());
    if (keep_symbols) {
      fst->SetInputSymbols(symbols_);
      fst->SetOutputSymbols(symbols_);
    }
    if (compiler_(content_, fst.get())) {
      return fst.release();
    } else {
      return nullptr;
    }
  }

  CompactStringFst<Arc> *GetCompactFst(bool keep_symbols = false) {
    std::unique_ptr<CompactStringFst<Arc>> fst;
    if (keep_symbols) {
      VectorFst<Arc> tmp;
      tmp.SetInputSymbols(symbols_);
      tmp.SetOutputSymbols(symbols_);
      fst.reset(new CompactStringFst<Arc>(tmp));
    } else {
      fst.reset(new CompactStringFst<Arc>());
    }
    if (compiler_(content_, fst.get())) {
      return fst.release();
    } else {
      return nullptr;
    }
  }

 private:
  size_t nline_;
  std::istream &istrm_;
  std::string source_;
  EntryType entry_type_;
  StringTokenType token_type_;
  const SymbolTable *symbols_;
  bool done_;
  StringCompiler<Arc> compiler_;
  std::string content_;  // The actual content of the input stream's next FST.

  StringReader(const StringReader &) = delete;
  StringReader &operator=(const StringReader &) = delete;
};

// Computes the minimal length required to encode each line number as a decimal
// number, or zero if the file is not seekable.
int KeySize(const char *source);

template <class Arc>
void FarCompileStrings(const std::vector<std::string> &in_sources,
                       const std::string &out_source,
                       const std::string &fst_type, const FarType &far_type,
                       int32 generate_keys, FarEntryType fet, FarTokenType tt,
                       const std::string &symbols_source,
                       const std::string &unknown_symbol, bool keep_symbols,
                       bool initial_symbols, bool allow_negative_labels,
                       const std::string &key_prefix,
                       const std::string &key_suffix) {
  typename StringReader<Arc>::EntryType entry_type;
  if (fet == FET_LINE) {
    entry_type = StringReader<Arc>::LINE;
  } else if (fet == FET_FILE) {
    entry_type = StringReader<Arc>::FILE;
  } else {
    FSTERROR() << "FarCompileStrings: Unknown entry type";
    return;
  }
  StringTokenType token_type;
  if (tt == FTT_SYMBOL) {
    token_type = StringTokenType::SYMBOL;
  } else if (tt == FTT_BYTE) {
    token_type = StringTokenType::BYTE;
  } else if (tt == FTT_UTF8) {
    token_type = StringTokenType::UTF8;
  } else {
    FSTERROR() << "FarCompileStrings: Unknown token type";
    return;
  }
  bool compact;
  if (fst_type.empty() || (fst_type == "vector")) {
    compact = false;
  } else if (fst_type == "compact") {
    compact = true;
  } else {
    FSTERROR() << "FarCompileStrings: Unknown FST type: " << fst_type;
    return;
  }
  std::unique_ptr<const SymbolTable> syms;
  typename Arc::Label unknown_label = kNoLabel;
  if (!symbols_source.empty()) {
    const SymbolTableTextOptions opts(allow_negative_labels);
    syms.reset(SymbolTable::ReadText(symbols_source, opts));
    if (!syms) {
      LOG(ERROR) << "FarCompileStrings: Error reading symbol table: "
                 << symbols_source;
      return;
    }
    if (!unknown_symbol.empty()) {
      unknown_label = syms->Find(unknown_symbol);
      if (unknown_label == kNoLabel) {
        FSTERROR() << "FarCompileStrings: Label \"" << unknown_label
                   << "\" missing from symbol table: " << symbols_source;
        return;
      }
    }
  }
  std::unique_ptr<FarWriter<Arc>> far_writer(
      FarWriter<Arc>::Create(out_source, far_type));
  if (!far_writer) return;
  int n = 0;
  for (const auto &in_source : in_sources) {
    // Don't try to call KeySize("").
    if (generate_keys == 0 && in_source.empty()) {
      FSTERROR() << "FarCompileStrings: Read from a file instead of stdin or"
                 << " set the --generate_keys flag.";
      return;
    }
    const int key_size = generate_keys ? generate_keys
                                       : (entry_type == StringReader<Arc>::FILE
                                              ? 1
                                              : KeySize(in_source.c_str()));
    if (key_size == 0) {
      FSTERROR() << "FarCompileStrings: " << in_source << " is not seekable.  "
                 << "Read from a file instead or set the --generate_keys flag.";
      return;
    }
    std::ifstream fstrm;
    if (!in_source.empty()) {
      fstrm.open(in_source);
      if (!fstrm) {
        FSTERROR() << "FarCompileStrings: Can't open file: " << in_source;
        return;
      }
    }
    std::istream &istrm = fstrm.is_open() ? fstrm : std::cin;
    bool keep_syms = keep_symbols;
    for (StringReader<Arc> reader(
             istrm, in_source.empty() ? "stdin" : in_source, entry_type,
             token_type, allow_negative_labels, syms.get(), unknown_label);
         !reader.Done(); reader.Next()) {
      ++n;
      std::unique_ptr<const Fst<Arc>> fst;
      if (compact) {
        fst.reset(reader.GetCompactFst(keep_syms));
      } else {
        fst.reset(reader.GetVectorFst(keep_syms));
      }
      if (initial_symbols) keep_syms = false;
      if (!fst) {
        FSTERROR() << "FarCompileStrings: Compiling string number " << n
                   << " in file " << in_source << " failed with token_type = "
                   << (tt == FTT_BYTE
                           ? "byte"
                           : (tt == FTT_UTF8
                                  ? "utf8"
                                  : (tt == FTT_SYMBOL ? "symbol" : "unknown")))
                   << " and entry_type = "
                   << (fet == FET_LINE
                           ? "line"
                           : (fet == FET_FILE ? "file" : "unknown"));
        return;
      }
      std::ostringstream keybuf;
      keybuf.width(key_size);
      keybuf.fill('0');
      keybuf << n;
      std::string key;
      if (generate_keys > 0) {
        key = keybuf.str();
      } else {
        auto *source = new char[in_source.size() + 1];
        strcpy(source, in_source.c_str());  // NOLINT
        key = basename(source);
        if (entry_type != StringReader<Arc>::FILE) {
          key += "-";
          key += keybuf.str();
        }
        delete[] source;
      }
      far_writer->Add(key_prefix + key + key_suffix, *fst);
    }
    if (generate_keys == 0) n = 0;
  }
}

}  // namespace fst

#endif  // FST_EXTENSIONS_FAR_COMPILE_STRINGS_H_
