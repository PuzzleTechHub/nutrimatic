// compile-main.h

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
// Author: riley@google.com (Michael Riley)
//
// \file
// Classes and functions to compile a binary Fst from textual input.
// Includes helper function for fstcompile.cc that templates the main
// on the arc type to support multiple and extensible arc types.

#ifndef FST_COMPILE_MAIN_H__
#define FST_COMPILE_MAIN_H__

#include <tr1/unordered_map>
using std::tr1::unordered_map;
#include <sstream>
#include <string>
#include <vector>

#include <fst/vector-fst.h>
#include <fst/main.h>

DECLARE_bool(acceptor);
DECLARE_string(arc_type);
DECLARE_string(fst_type);

DECLARE_string(isymbols);
DECLARE_string(osymbols);
DECLARE_string(ssymbols);

DECLARE_bool(keep_isymbols);
DECLARE_bool(keep_osymbols);
DECLARE_bool(keep_state_numbering);

DECLARE_bool(allow_negative_labels);  // not recommended; may cause conflicts

namespace fst {

template <class A> class FstReader {
 public:
  typedef A Arc;
  typedef typename A::StateId StateId;
  typedef typename A::Label Label;
  typedef typename A::Weight Weight;

  FstReader(istream &istrm, const string &source,
            const SymbolTable *isyms, const SymbolTable *osyms,
            const SymbolTable *ssyms, bool accep, bool ikeep,
            bool okeep, bool nkeep)
      : nline_(0), source_(source),
        isyms_(isyms), osyms_(osyms), ssyms_(ssyms),
        nstates_(0), keep_state_numbering_(nkeep) {
    char line[kLineLen];
    while (istrm.getline(line, kLineLen)) {
      ++nline_;
      vector<char *> col;
      SplitToVector(line, "\n\t ", &col, true);
      if (col.size() == 0 || col[0][0] == '\0')  // empty line
        continue;
      if (col.size() > 5 ||
          (col.size() > 4 && accep) ||
          (col.size() == 3 && !accep)) {
        LOG(ERROR) << "FstReader: Bad number of columns, source = " << source_
                   << ", line = " << nline_;
        exit(1);
      }
      StateId s = StrToStateId(col[0]);
      while (s >= fst_.NumStates())
        fst_.AddState();
      if (nline_ == 1)
        fst_.SetStart(s);

      Arc arc;
      StateId d = s;
      switch (col.size()) {
      case 1:
        fst_.SetFinal(s, Weight::One());
        break;
      case 2:
        fst_.SetFinal(s, StrToWeight(col[1], true));
        break;
      case 3:
        arc.nextstate = d = StrToStateId(col[1]);
        arc.ilabel = StrToILabel(col[2]);
        arc.olabel = arc.ilabel;
        arc.weight = Weight::One();
        fst_.AddArc(s, arc);
        break;
      case 4:
        arc.nextstate = d = StrToStateId(col[1]);
        arc.ilabel = StrToILabel(col[2]);
        if (accep) {
          arc.olabel = arc.ilabel;
          arc.weight = StrToWeight(col[3], false);
        } else {
          arc.olabel = StrToOLabel(col[3]);
          arc.weight = Weight::One();
        }
        fst_.AddArc(s, arc);
        break;
      case 5:
        arc.nextstate = d = StrToStateId(col[1]);
        arc.ilabel = StrToILabel(col[2]);
        arc.olabel = StrToOLabel(col[3]);
        arc.weight = StrToWeight(col[4], false);
        fst_.AddArc(s, arc);
      }
      while (d >= fst_.NumStates())
        fst_.AddState();
    }
    if (ikeep)
      fst_.SetInputSymbols(isyms);
    if (okeep)
      fst_.SetOutputSymbols(osyms);
  }

  const VectorFst<A> &Fst() const { return fst_; }

 private:
  // Maximum line length in text file.
  static const int kLineLen = 8096;

  int64 StrToId(const char *s, const SymbolTable *syms,
                const char *name, bool allow_negative = false) const {
    int64 n;

    if (syms) {
      n = syms->Find(s);
      if (n == -1 || (!allow_negative && n < 0)) {
        LOG(ERROR) << "FstReader: Symbol \"" << s
                   << "\" is not mapped to any integer " << name
                   << ", symbol table = " << syms->Name()
                   << ", source = " << source_ << ", line = " << nline_;
        exit(1);
      }
    } else {
      char *p;
      n = strtoll(s, &p, 10);
      if (p < s + strlen(s) || (!allow_negative && n < 0)) {
        LOG(ERROR) << "FstReader: Bad " << name << " integer = \"" << s
                   << "\", source = " << source_ << ", line = " << nline_;
        exit(1);
      }
    }
    return n;
  }

  StateId StrToStateId(const char *s) {
    StateId n = StrToId(s, ssyms_, "state ID");

    if (keep_state_numbering_)
      return n;

    // remap state IDs to make dense set
    typename unordered_map<StateId, StateId>::const_iterator it = states_.find(n);
    if (it == states_.end()) {
      states_[n] = nstates_;
      return nstates_++;
    } else {
      return it->second;
    }
  }

  StateId StrToILabel(const char *s) const {
    return StrToId(s, isyms_, "arc ilabel", FLAGS_allow_negative_labels);
  }

  StateId StrToOLabel(const char *s) const {
    return StrToId(s, osyms_, "arc olabel", FLAGS_allow_negative_labels);
  }

  Weight StrToWeight(const char *s, bool allow_zero) const {
    Weight w;
    istringstream strm(s);
    strm >> w;
    if (!strm || (!allow_zero && w == Weight::Zero())) {
      LOG(ERROR) << "FstReader: Bad weight = \"" << s
                 << "\", source = " << source_ << ", line = " << nline_;
      exit(1);
    }
    return w;
  }

  VectorFst<A> fst_;
  size_t nline_;
  string source_;                      // text FST source name
  const SymbolTable *isyms_;           // ilabel symbol table
  const SymbolTable *osyms_;           // olabel symbol table
  const SymbolTable *ssyms_;           // slabel symbol table
  unordered_map<StateId, StateId> states_;  // state ID map
  StateId nstates_;                    // number of seen states
  bool keep_state_numbering_;
  DISALLOW_COPY_AND_ASSIGN(FstReader);
};

// Main function for fstcompile templated on the arc type.  Last two
// arguments unneeded since fstcompile passes the arc type as a flag
// unlike the other mains, which infer the arc type from an input Fst.
template <class Arc>
int CompileMain(int argc, char **argv, istream & /* istrm */,
                const FstReadOptions & /* opts */) {
  const char *source = "standard input";
  istream *istrm = &std::cin;
  if (argc > 1 && strcmp(argv[1], "-") != 0) {
    source = argv[1];
    istrm = new ifstream(argv[1]);
    if (!istrm) {
      LOG(ERROR) << argv[0] << ": Open failed, file = " << argv[1];
      return 1;
    }
  }
  const SymbolTable *isyms = 0, *osyms = 0, *ssyms = 0;

  if (!FLAGS_isymbols.empty()) {
    isyms = SymbolTable::ReadText(FLAGS_isymbols, FLAGS_allow_negative_labels);
    if (!isyms) exit(1);
  }

  if (!FLAGS_osymbols.empty()) {
    osyms = SymbolTable::ReadText(FLAGS_osymbols, FLAGS_allow_negative_labels);
    if (!osyms) exit(1);
  }

  if (!FLAGS_ssymbols.empty()) {
    ssyms = SymbolTable::ReadText(FLAGS_ssymbols);
    if (!ssyms) exit(1);
  }

  FstReader<Arc> fstreader(*istrm, source, isyms, osyms, ssyms,
                           FLAGS_acceptor, FLAGS_keep_isymbols,
                           FLAGS_keep_osymbols, FLAGS_keep_state_numbering);

  const Fst<Arc> *fst = &fstreader.Fst();
  if (FLAGS_fst_type != "vector") {
    fst = Convert<Arc>(*fst, FLAGS_fst_type);
    if (!fst) return 1;
  }
  fst->Write(argc > 2 ? argv[2] : "");
  if (istrm != &std::cin)
    delete istrm;
  return 0;
}

}  // namespace fst;

#endif  // FST_COMPILE_MAIN_H__
