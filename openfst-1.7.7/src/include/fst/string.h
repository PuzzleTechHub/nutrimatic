// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Utilities to convert strings into FSTs.

#ifndef FST_STRING_H_
#define FST_STRING_H_

#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include <fst/flags.h>
#include <fst/types.h>
#include <fst/log.h>

#include <fst/compact-fst.h>
#include <fst/icu.h>
#include <fst/mutable-fst.h>
#include <fst/properties.h>
#include <fst/symbol-table.h>
#include <fst/util.h>


DECLARE_string(fst_field_separator);

namespace fst {

enum StringTokenType { SYMBOL = 1, BYTE = 2, UTF8 = 3 };

enum EpsilonSymbolPrintType { NONEPS_SYMBOLS = 1, SYMBOLS_INCL_EPS = 2 };

namespace internal {

template <class Label>
bool ConvertSymbolToLabel(const char *str, const SymbolTable *syms,
                          Label unknown_label, bool allow_negative,
                          Label *output) {
  int64 n;
  if (syms) {
    n = syms->Find(str);
    if ((n == kNoSymbol) && (unknown_label != kNoLabel)) n = unknown_label;
    if (n == kNoSymbol || (!allow_negative && n < 0)) {
      LOG(ERROR) << "ConvertSymbolToLabel: Symbol \"" << str
                 << "\" is not mapped to any integer label, symbol table = "
                 << syms->Name();
      return false;
    }
  } else {
    char *p;
    n = strtoll(str, &p, 10);
    if (p < str + strlen(str) || (!allow_negative && n < 0)) {
      LOG(ERROR) << "ConvertSymbolToLabel: Bad label integer "
                 << "= \"" << str << "\"";
      return false;
    }
  }
  *output = n;
  return true;
}

template <class Label>
bool ConvertStringToLabels(const std::string &str, StringTokenType token_type,
                           const SymbolTable *syms, Label unknown_label,
                           bool allow_negative, std::vector<Label> *labels,
                           const std::string &sep = FLAGS_fst_field_separator) {
  labels->clear();
  if (token_type == StringTokenType::BYTE) {
    labels->reserve(str.size());
    return ByteStringToLabels(str, labels);
  } else if (token_type == StringTokenType::UTF8) {
    return UTF8StringToLabels(str, labels);
  } else {
    std::unique_ptr<char[]> c_str(new char[str.size() + 1]);
    str.copy(c_str.get(), str.size());
    c_str[str.size()] = 0;
    std::vector<char *> vec;
    const std::string separator = "\n" + sep;
    SplitString(c_str.get(), separator.c_str(), &vec, true);
    for (const char *c : vec) {
      Label label;
      if (!ConvertSymbolToLabel(c, syms, unknown_label, allow_negative,
                                &label)) {
        return false;
      }
      labels->push_back(label);
    }
  }
  return true;
}

// The last character of 'sep' is used as a separator between symbols.
// Additionally, epsilon symbols will be printed only if the epsilon symbol
// print type is set to SYMBOLS_INCL_EPS, and will be ignored (default) if set
// to NONEPS_SYMBOLS.
template <class Label>
bool LabelsToSymbolString(const std::vector<Label> &labels, std::string *str,
                          const SymbolTable &syms,
                          const std::string &sep = FLAGS_fst_field_separator,
                          EpsilonSymbolPrintType eps_sym_print_type =
                              EpsilonSymbolPrintType::NONEPS_SYMBOLS) {
  std::stringstream ostrm;
  std::string delim = "";
  for (auto label : labels) {
    // Don't include epsilon labels in output if in NONEPS_SYMBOLS mode.
    if (!label && eps_sym_print_type == EpsilonSymbolPrintType::NONEPS_SYMBOLS)
      continue;
    ostrm << delim;
    const std::string &symbol = syms.Find(label);
    if (symbol.empty()) {
      LOG(ERROR) << "LabelsToSymbolString: Label " << label
                 << " is not mapped onto any textual symbol in symbol table "
                 << syms.Name();
      return false;
    }
    ostrm << symbol;
    delim = std::string(1, sep.back());
  }
  *str = ostrm.str();
  return !!ostrm;
}

// The last character of 'sep' is used as a separator between symbols.
// Additionally, epsilon symbols will be printed only if the epsilon symbol
// print type is set to SYMBOLS_INCL_EPS, and will be ignored (default) if set
// to NONEPS_SYMBOLS.
template <class Label>
bool LabelsToNumericString(const std::vector<Label> &labels, std::string *str,
                           const std::string &sep = FLAGS_fst_field_separator,
                           EpsilonSymbolPrintType eps_sym_print_type =
                               EpsilonSymbolPrintType::NONEPS_SYMBOLS) {
  std::stringstream ostrm;
  std::string delim = "";
  for (auto label : labels) {
    // Don't include epsilon labels in output if in NONEPS_SYMBOLS mode.
    if (!label && eps_sym_print_type == EpsilonSymbolPrintType::NONEPS_SYMBOLS)
      continue;
    ostrm << delim;
    ostrm << label;
    delim = std::string(1, sep.back());
  }
  *str = ostrm.str();
  return !!ostrm;
}

}  // namespace internal

// Functor for compiling a string in an FST.
template <class Arc>
class StringCompiler {
 public:
  using Label = typename Arc::Label;
  using StateId = typename Arc::StateId;
  using Weight = typename Arc::Weight;

  explicit StringCompiler(StringTokenType token_type = BYTE,
                          const SymbolTable *syms = nullptr,
                          Label unknown_label = kNoLabel,
                          bool allow_negative = false)
      : token_type_(token_type),
        syms_(syms),
        unknown_label_(unknown_label),
        allow_negative_(allow_negative) {}

  // Compiles string into an FST. With SYMBOL token type, sep is used to
  // specify the set of char separators between symbols, in addition
  // of '\n' which is always treated as a separator.
  // Returns true on success.
  template <class FST>
  bool operator()(const std::string &str, FST *fst,
                  const std::string &sep = FLAGS_fst_field_separator) const {
    std::vector<Label> labels;
    if (!internal::ConvertStringToLabels(str, token_type_, syms_,
                                         unknown_label_, allow_negative_,
                                         &labels, sep)) {
      return false;
    }
    Compile(labels, fst);
    return true;
  }

  // Same as above but allows to specify a weight for the string.
  template <class FST>
  bool operator()(const std::string &str, FST *fst, Weight weight,
                  const std::string &sep = FLAGS_fst_field_separator) const {
    std::vector<Label> labels;
    if (!internal::ConvertStringToLabels(str, token_type_, syms_,
                                         unknown_label_, allow_negative_,
                                         &labels, sep)) {
      return false;
    }
    Compile(labels, fst, std::move(weight));
    return true;
  }

 private:
  void Compile(const std::vector<Label> &labels, MutableFst<Arc> *fst,
               Weight weight = Weight::One()) const {
    fst->DeleteStates();
    auto state = fst->AddState();
    fst->SetStart(state);
    fst->AddStates(labels.size());
    for (auto label : labels) {
      fst->AddArc(state, Arc(label, label, state + 1));
      ++state;
    }
    fst->SetFinal(state, std::move(weight));
    fst->SetProperties(kCompiledStringProperties, kCompiledStringProperties);
  }

  template <class Unsigned>
  void Compile(const std::vector<Label> &labels,
               CompactStringFst<Arc, Unsigned> *fst) const {
    using Compactor = typename CompactStringFst<Arc, Unsigned>::Compactor;
    fst->SetCompactor(
        std::make_shared<Compactor>(labels.begin(), labels.end()));
  }

  template <class Unsigned>
  void Compile(const std::vector<Label> &labels,
               CompactWeightedStringFst<Arc, Unsigned> *fst,
               Weight weight = Weight::One()) const {
    std::vector<std::pair<Label, Weight>> compacts;
    compacts.reserve(labels.size() + 1);
    for (StateId i = 0; i < static_cast<StateId>(labels.size()) - 1; ++i) {
      compacts.emplace_back(labels[i], Weight::One());
    }
    compacts.emplace_back(!labels.empty() ? labels.back() : kNoLabel, weight);
    using Compactor =
        typename CompactWeightedStringFst<Arc, Unsigned>::Compactor;
    fst->SetCompactor(
        std::make_shared<Compactor>(compacts.begin(), compacts.end()));
  }

  const StringTokenType token_type_;
  const SymbolTable *syms_;    // Symbol table (used when token type is symbol).
  const Label unknown_label_;  // Label for token missing from symbol table.
  const bool allow_negative_;  // Negative labels allowed?

  StringCompiler(const StringCompiler &) = delete;
  StringCompiler &operator=(const StringCompiler &) = delete;
};

// A useful alias when using StdArc.
using StdStringCompiler = StringCompiler<StdArc>;

// Helpers for StringPrinter.

// Converts an FST to a vector of output labels. To get input labels, use
// Project or Invert. Returns true on success. Use only with string FSTs; may
// loop for non-string FSTs.
template <class Arc>
bool StringFstToOutputLabels(const Fst<Arc> &fst,
                             std::vector<typename Arc::Label> *labels) {
  labels->clear();
  auto s = fst.Start();
  if (s == kNoStateId) {
    LOG(ERROR) << "StringFstToOutputLabels: Invalid start state";
    return false;
  }
  while (fst.Final(s) == Arc::Weight::Zero()) {
    ArcIterator<Fst<Arc>> aiter(fst, s);
    if (aiter.Done()) {
      LOG(ERROR) << "StringFstToOutputLabels: Does not reach final state";
      return false;
    }
    const auto &arc = aiter.Value();
    labels->push_back(arc.olabel);
    s = arc.nextstate;
    aiter.Next();
    if (!aiter.Done()) {
      LOG(ERROR) << "StringFstToOutputLabels: State " << s
                 << " has multiple outgoing arcs";
      return false;
    }
  }
  if (fst.NumArcs(s) != 0) {
    LOG(ERROR) << "StringFstToOutputLabels: Final state " << s
               << " has outgoing arc(s)";
    return false;
  }
  return true;
}

// Converts a list of symbols to a string. If the token type is SYMBOL, the last
// character of sep is used to separate textual symbols. Additionally, if the
// token type is SYMBOL, then epsilon symbols will be printed only if the
// epsilon symbol print type is set to SYMBOLS_INCL_EPS, and will be ignored
// (default) if set to NONEPS_SYMBOLS. Returns true on success.
template <class Label>
bool LabelsToString(const std::vector<Label> &labels, std::string *str,
                    StringTokenType ttype = BYTE,
                    const SymbolTable *syms = nullptr,
                    const std::string &sep = FLAGS_fst_field_separator,
                    EpsilonSymbolPrintType eps_sym_print_type =
                        EpsilonSymbolPrintType::NONEPS_SYMBOLS) {
  switch (ttype) {
    case StringTokenType::BYTE: {
      return LabelsToByteString(labels, str);
    }
    case StringTokenType::UTF8: {
      return LabelsToUTF8String(labels, str);
    }
    case StringTokenType::SYMBOL: {
      return syms ? internal::LabelsToSymbolString(labels, str, *syms, sep,
                                                   eps_sym_print_type)
                  : internal::LabelsToNumericString(labels, str, sep,
                                                    eps_sym_print_type);
    }
  }
  return false;
}

// Functor for printing a string FST as a string.
template <class Arc>
class StringPrinter {
 public:
  using Label = typename Arc::Label;

  explicit StringPrinter(StringTokenType token_type = BYTE,
                         const SymbolTable *syms = nullptr,
                         EpsilonSymbolPrintType eps_sym_print_type =
                             EpsilonSymbolPrintType::NONEPS_SYMBOLS)
      : token_type_(token_type),
        syms_(syms),
        eps_sym_print_type_(eps_sym_print_type) {}

  // Converts the FST into a string. With SYMBOL token type, the last character
  // of sep is used as a separator between symbols. Returns true on success.
  bool operator()(const Fst<Arc> &fst, std::string *str,
                  const std::string &sep = FLAGS_fst_field_separator) const {
    std::vector<Label> labels;
    return (StringFstToOutputLabels(fst, &labels) &&
            LabelsToString(labels, str, token_type_, syms_, sep,
                           eps_sym_print_type_));
  }

 private:
  const StringTokenType token_type_;
  const SymbolTable *syms_;
  const EpsilonSymbolPrintType
      eps_sym_print_type_;  // Whether to print epsilons in
                            // StringTokenType::SYMBOL mode.

  StringPrinter(const StringPrinter &) = delete;
  StringPrinter &operator=(const StringPrinter &) = delete;
};

// A useful alias when using StdArc.
using StdStringPrinter = StringPrinter<StdArc>;

}  // namespace fst

#endif  // FST_STRING_H_
