// info-main.h

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
// Classes and functions to print out various information about binary
// FSTs such as number of states and arcs and property values (see
// properties.h).  Includes helper function for fstinfo.cc that
// templates the main on the arc type to support multiple and
// extensible arc types.

#ifndef FST_INFO_MAIN_H__
#define FST_INFO_MAIN_H__

#include <cstdio>
#include <string>
#include <vector>
#include <fst/main.h>
#include <fst/connect.h>
#include <fst/dfs-visit.h>
#include <fst/fst.h>
#include <fst/test-properties.h>

DECLARE_string(arc_filter);
DECLARE_bool(pipe);
DECLARE_bool(test_properties);

namespace fst {

template <class A> class FstInfo {
 public:
  typedef A Arc;
  typedef typename A::StateId StateId;
  typedef typename A::Label Label;
  typedef typename A::Weight Weight;

  FstInfo(const Fst<A> &fst, bool test_properties,
          string arc_filter_type = "any")
      : fst_type_(fst.Type()),
        input_symbols_(fst.InputSymbols() ?
                       fst.InputSymbols()->Name() : "none"),
        output_symbols_(fst.OutputSymbols() ?
                        fst.OutputSymbols()->Name() : "none"),
        nstates_(0), narcs_(0), start_(fst.Start()), nfinal_(0),
        nepsilons_(0), niepsilons_(0), noepsilons_(0),
        naccess_(0), ncoaccess_(0), nconnect_(0), nscc_(0),
        properties_(fst.Properties(kFstProperties, test_properties)) {
    for (StateIterator< Fst<A> > siter(fst);
         !siter.Done();
         siter.Next()) {
      ++nstates_;
      StateId s = siter.Value();
      if (fst.Final(s) != Weight::Zero())
        ++nfinal_;
      for (ArcIterator< Fst<A> > aiter(fst, s);
           !aiter.Done();
           aiter.Next()) {
        const A &arc = aiter.Value();
        ++narcs_;
        if (arc.ilabel == 0 && arc.olabel == 0)
          ++nepsilons_;
        if (arc.ilabel == 0)
          ++niepsilons_;
        if (arc.olabel == 0)
          ++noepsilons_;
      }
    }
    vector<StateId> scc;
    vector<bool> access, coaccess;
    uint64 props = 0;
    SccVisitor<Arc> scc_visitor(&scc, &access, &coaccess, &props);
    if (arc_filter_type == "any")
      DfsVisit(fst, &scc_visitor);
    else if (arc_filter_type == "epsilon")
      DfsVisit(fst, &scc_visitor, EpsilonArcFilter<Arc>());
    else if (arc_filter_type == "iepsilon")
      DfsVisit(fst, &scc_visitor, InputEpsilonArcFilter<Arc>());
    else if (arc_filter_type == "oepsilon")
      DfsVisit(fst, &scc_visitor, OutputEpsilonArcFilter<Arc>());
    else
      LOG(FATAL) << "Bad arc filter type: " << arc_filter_type;


    for (StateId s = 0; s < scc.size(); ++s) {
      if (access[s])
        ++naccess_;
      if (coaccess[s])
        ++ncoaccess_;
      if (access[s] && coaccess[s])
        ++nconnect_;
      if (scc[s] >= nscc_)
        nscc_ = scc[s] + 1;
    }
  }
  const string& FstType() const { return fst_type_; }
  const string& ArcType() const { return A::Type(); }
  const string& InputSymbols() const { return input_symbols_; }
  const string& OutputSymbols() const { return output_symbols_; }
  int64 NumStates() const { return nstates_; }
  int64 NumArcs() const { return narcs_; }
  int64 Start() const { return start_; }
  int64 NumFinal() const { return nfinal_; }
  int64 NumEpsilons() const { return nepsilons_; }
  int64 NumInputEpsilons() const { return niepsilons_; }
  int64 NumOutputEpsilons() const { return noepsilons_; }
  int64 NumAccessible() const { return naccess_; }
  int64 NumCoAccessible() const { return ncoaccess_; }
  int64 NumConnected() const { return nconnect_; }
  int64 NumScc() const { return nscc_; }
  uint64 Properties() const { return properties_; }

 private:
  string fst_type_;
  string input_symbols_;
  string output_symbols_;
  int64 nstates_;
  int64 narcs_;
  int64 start_;
  int64 nfinal_;
  int64 nepsilons_;
  int64 niepsilons_;
  int64 noepsilons_;
  int64 naccess_;
  int64 ncoaccess_;
  int64 nconnect_;
  int64 nscc_;
  uint64 properties_;
  DISALLOW_COPY_AND_ASSIGN(FstInfo);
};

template <class A>
void PrintFstInfo(const FstInfo<A> &fstinfo, bool pipe = false,
                  string arc_filter_type = "any") {
  FILE *fp = pipe ? stderr : stdout;

  fprintf(fp, "%-50s%s\n", "fst type",
          fstinfo.FstType().c_str());
    fprintf(fp, "%-50s%s\n", "arc type",
            fstinfo.ArcType().c_str());
  fprintf(fp, "%-50s%s\n", "input symbol table",
          fstinfo.InputSymbols().c_str());
  fprintf(fp, "%-50s%s\n", "output symbol table",
          fstinfo.OutputSymbols().c_str());
  fprintf(fp, "%-50s%lld\n", "# of states",
          fstinfo.NumStates());
  fprintf(fp, "%-50s%lld\n", "# of arcs",
          fstinfo.NumArcs());
  fprintf(fp, "%-50s%lld\n", "initial state",
          fstinfo.Start());
  fprintf(fp, "%-50s%lld\n", "# of final states",
          fstinfo.NumFinal());
  fprintf(fp, "%-50s%lld\n", "# of input/output epsilons",
          fstinfo.NumEpsilons());
  fprintf(fp, "%-50s%lld\n", "# of input epsilons",
          fstinfo.NumInputEpsilons());
  fprintf(fp, "%-50s%lld\n", "# of output epsilons",
          fstinfo.NumOutputEpsilons());

  string arc_type = "";
  if (arc_filter_type == "epsilon")
    arc_type = "epsilon ";
  else if (arc_filter_type == "iepsilon")
    arc_type = "input-epsilon ";
  else if (arc_filter_type == "oepsilon")
    arc_type = "output-epsilon ";

  string accessible_label = "# of " +  arc_type + "accessible states";
  fprintf(fp, "%-50s%lld\n", accessible_label.c_str(),
          fstinfo.NumAccessible());
  string coaccessible_label = "# of " +  arc_type + "coaccessible states";
  fprintf(fp, "%-50s%lld\n", coaccessible_label.c_str(),
          fstinfo.NumCoAccessible());
  string connected_label = "# of " +  arc_type + "connected states";
  fprintf(fp, "%-50s%lld\n", connected_label.c_str(),
          fstinfo.NumConnected());
  string numscc_label = "# of " +  arc_type + "strongly conn components";
  fprintf(fp, "%-50s%lld\n", numscc_label.c_str(),
          fstinfo.NumScc());

  uint64 prop = 1;
  for (int i = 0; i < 64; ++i, prop <<= 1) {
    if (prop & kBinaryProperties) {
      char value = 'n';
      if (fstinfo.Properties() & prop) value = 'y';
      fprintf(fp, "%-50s%c\n", PropertyNames[i], value);
    } else if (prop & kPosTrinaryProperties) {
      char value = '?';
      if (fstinfo.Properties() & prop) value = 'y';
      else if (fstinfo.Properties() & prop << 1) value = 'n';
      fprintf(fp, "%-50s%c\n", PropertyNames[i], value);
    }
  }
}

// Main function for fstinfo templated on the arc type.
template <class Arc>
int InfoMain(int argc, char **argv, istream &strm,
             const FstReadOptions &opts) {
  Fst<Arc> *fst = Fst<Arc>::Read(strm, opts);
  if (!fst) return 1;
  FstInfo<Arc> fstinfo(*fst, FLAGS_test_properties, FLAGS_arc_filter);
  PrintFstInfo<Arc>(fstinfo, FLAGS_pipe, FLAGS_arc_filter);
  if (FLAGS_pipe)
    fst->Write("");
  return 0;
}

}  // namespace fst

#endif  // INFO_MAIN_H
