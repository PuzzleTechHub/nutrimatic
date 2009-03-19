// draw-main.h

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
// Author: allauzen@google.com (Cyril Allauzen)
//
// \file
// Classes and functions to draw a binary FSTs using dot.
// Includes helper function for fstdraw.cc that templates the main on
// the arc type to support multiple and extensible arc types.

#ifndef FST_DRAW_MAIN_H__
#define FST_DRAW_MAIN_H__

#include <sstream>
#include <string>

#include <fst/main.h>
#include <fst/fst.h>

DECLARE_bool(acceptor);
DECLARE_string(isymbols);
DECLARE_string(osymbols);
DECLARE_string(ssymbols);

DECLARE_bool(numeric);

DECLARE_string(save_isymbols);
DECLARE_string(save_osymbols);

DECLARE_string(title);
DECLARE_bool(portrait);
DECLARE_bool(vertical);
DECLARE_int32(fontsize);
DECLARE_double(width);
DECLARE_double(height);
DECLARE_double(nodesep);
DECLARE_double(ranksep);
DECLARE_int32(precision);
DECLARE_bool(show_weight_one);

DECLARE_bool(allow_negative_labels);  // not recommended; may cause conflicts

namespace fst {

template <class A> class FstDrawer {
 public:
  typedef A Arc;
  typedef typename A::StateId StateId;
  typedef typename A::Label Label;
  typedef typename A::Weight Weight;

  FstDrawer(const Fst<A> &fst,
            const SymbolTable *isyms,
            const SymbolTable *osyms,
            const SymbolTable *ssyms,
            bool accep,
            string title,
            float width,
            float height,
            bool portrait,
            bool vertical,
            float ranksep,
            float nodesep,
            int fontsize,
            int precision,
            bool show_weight_one)
      : fst_(fst), isyms_(isyms), osyms_(osyms), ssyms_(ssyms),
        accep_(accep && fst.Properties(kAcceptor, true)), ostrm_(0),
        title_(title), width_(width), height_(height), portrait_(portrait),
        vertical_(vertical), ranksep_(ranksep), nodesep_(nodesep),
        fontsize_(fontsize), precision_(precision),
        show_weight_one_(show_weight_one) {}

  // Draw Fst to an output buffer (or stdout if buf = 0)
  void Draw(ostream *strm, const string &dest) {
    ostrm_ = strm;
    dest_ = dest;
    StateId start = fst_.Start();
    if (start == kNoStateId)
      return;

    PrintString("digraph FST {\n");
    if (vertical_)
      PrintString("rankdir = BT;\n");
    else
      PrintString("rankdir = LR;\n");
    PrintString("size = \"");
    Print(width_);
    PrintString(",");
    Print(height_);
    PrintString("\";\n");
    if (!dest_.empty())
      PrintString("label = \"" + title_ + "\";\n");
    PrintString("center = 1;\n");
    if (portrait_)
      PrintString("orientation = Portrait;\n");
    else
      PrintString("orientation = Landscape;\n");
    PrintString("ranksep = \"");
    Print(ranksep_);
    PrintString("\";\n");
    PrintString("nodesep = \"");
    Print(nodesep_);
    PrintString("\";\n");
    // initial state first
    DrawState(start);
    for (StateIterator< Fst<A> > siter(fst_);
         !siter.Done();
         siter.Next()) {
      StateId s = siter.Value();
      if (s != start)
        DrawState(s);
    }
    PrintString("}\n");
  }

 private:
  // Maximum line length in text file.
  static const int kLineLen = 8096;

  void PrintString(const string &s) const {
    *ostrm_ << s;
  }

  void PrintId(int64 id, const SymbolTable *syms,
               const char *name) const {
    if (syms) {
      string symbol = syms->Find(id);
      if (symbol == "") {
        LOG(ERROR) << "FstDrawer: Integer " << id
                   << " is not mapped to any textual symbol"
                   << ", symbol table = " << syms->Name()
                   << ", destination = " << dest_;
        exit(1);
      }
      PrintString(symbol);
    } else {
      char sid[kLineLen];
      snprintf(sid, kLineLen, "%lld", id);
      PrintString(sid);
    }
  }

  void PrintStateId(StateId s) const {
     PrintId(s, ssyms_, "state ID");
  }

  void PrintILabel(Label l) const {
     PrintId(l, isyms_, "arc input label");
  }

  void PrintOLabel(Label l) const {
     PrintId(l, osyms_, "arc output label");
  }

  template <class T>
  void Print(T t) const {
    *ostrm_ << t;
  }

  void DrawState(StateId s) const {
    Print(s);
    PrintString(" [label = \"");
    PrintStateId(s);
    Weight final = fst_.Final(s);
    if (final != Weight::Zero()) {
      if (show_weight_one_ || (final != Weight::One())) {
        PrintString("/");
        Print(final);
      }
      PrintString("\", shape = doublecircle,");
    } else {
      PrintString("\", shape = circle,");
    }
    if (s == fst_.Start())
      PrintString(" style = bold,");
    else
      PrintString(" style = solid,");
    PrintString(" fontsize = ");
    Print(fontsize_);
    PrintString("]\n");
    for (ArcIterator< Fst<A> > aiter(fst_, s);
         !aiter.Done();
         aiter.Next()) {
      Arc arc = aiter.Value();
      PrintString("\t");
      Print(s);
      PrintString(" -> ");
      Print(arc.nextstate);
      PrintString(" [label = \"");
      PrintILabel(arc.ilabel);
      if (!accep_) {
        PrintString(":");
        PrintOLabel(arc.olabel);
      }
      if (show_weight_one_ || (arc.weight != Weight::One())) {
        PrintString("/");
        Print(arc.weight);
      }
      PrintString("\", fontsize = ");
      Print(fontsize_);
      PrintString("];\n");
    }
  }

  const Fst<A> &fst_;
  const SymbolTable *isyms_;     // ilabel symbol table
  const SymbolTable *osyms_;     // olabel symbol table
  const SymbolTable *ssyms_;     // slabel symbol table
  bool accep_;                   // print as acceptor when possible
  ostream *ostrm_;               // drawn FST destination
  string dest_;                  // drawn FST destination name

  string title_;
  float width_;
  float height_;
  bool portrait_;
  bool vertical_;
  float ranksep_;
  float nodesep_;
  int fontsize_;
  int precision_;
  bool show_weight_one_;

  DISALLOW_COPY_AND_ASSIGN(FstDrawer);
};

// Main function for fstprint templated on the arc type.
template <class Arc>
int DrawMain(int argc, char **argv, istream &istrm,
              const FstReadOptions &opts) {
  Fst<Arc> *fst = Fst<Arc>::Read(istrm, opts);
  if (!fst) return 1;

  ostream *ostrm = &std::cout;
  string dest = "stdout";
  if (argc == 3) {
    dest = argv[2];
    ostrm = new ofstream(argv[2]);
    if (!*ostrm) {
      LOG(ERROR) << argv[0] << ": Open failed, file = " << argv[2];
      return 0;
    }
  }
  ostrm->precision(FLAGS_precision);

  const SymbolTable *isyms = 0, *osyms = 0, *ssyms = 0;

  if (!FLAGS_isymbols.empty() && !FLAGS_numeric) {
    isyms = SymbolTable::ReadText(FLAGS_isymbols, FLAGS_allow_negative_labels);
    if (!isyms) exit(1);
  }

  if (!FLAGS_osymbols.empty() && !FLAGS_numeric) {
    osyms = SymbolTable::ReadText(FLAGS_osymbols, FLAGS_allow_negative_labels);
    if (!osyms) exit(1);
  }

  if (!FLAGS_ssymbols.empty() && !FLAGS_numeric) {
    ssyms = SymbolTable::ReadText(FLAGS_ssymbols);
    if (!ssyms) exit(1);
  }

  if (!isyms && !FLAGS_numeric)
    isyms = fst->InputSymbols();
  if (!osyms && !FLAGS_numeric)
    osyms = fst->OutputSymbols();

  FstDrawer<Arc> fstdrawer(*fst, isyms, osyms, ssyms, FLAGS_acceptor,
                           FLAGS_title, FLAGS_width, FLAGS_height,
                           FLAGS_portrait, FLAGS_vertical,
                           FLAGS_ranksep, FLAGS_nodesep,
                           FLAGS_fontsize, FLAGS_precision,
                           FLAGS_show_weight_one);
  fstdrawer.Draw(ostrm, dest);

  if (isyms && !FLAGS_save_isymbols.empty())
    isyms->WriteText(FLAGS_save_isymbols);

  if (osyms && !FLAGS_save_osymbols.empty())
    osyms->WriteText(FLAGS_save_osymbols);

  if (ostrm != &std::cout)
    delete ostrm;
  return 0;
}

}  // namespace fst

#endif  // FST_DRAW_MAIN_H__
