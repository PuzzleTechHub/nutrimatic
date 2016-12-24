
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
// Copyright 2005-2010 Google, Inc.
// Author: jpr@google.com (Jake Ratkiewicz)
// Author: rws@google.com (Richard Sproat)
// Convenience file for including all MPDT operations at once, and/or
// registering them for new arc types.

#ifndef FST_EXTENSIONS_MPDT_MPDTSCRIPT_H_
#define FST_EXTENSIONS_MPDT_MPDTSCRIPT_H_

#include <utility>
using std::pair; using std::make_pair;
#include <vector>
using std::vector;

#include <fst/compose.h>  // for ComposeOptions
#include <fst/util.h>

#include <fst/script/fst-class.h>
#include <fst/script/arg-packs.h>
#include <fst/script/shortest-path.h>

#include <fst/extensions/mpdt/compose.h>
#include <fst/extensions/mpdt/expand.h>
#include <fst/extensions/mpdt/info.h>
#include <fst/extensions/mpdt/reverse.h>

namespace fst {
namespace script {

// MPDT COMPOSE

typedef args::Package<const FstClass &,
                      const FstClass &,
                      const vector<pair<int64, int64> >&,
                      const vector<int64>&,
                      MutableFstClass *,
                      const MPdtComposeOptions &,
                      bool> MPdtComposeArgs;

template<class Arc>
void MPdtCompose(MPdtComposeArgs *args) {
  const Fst<Arc> &ifst1 = *(args->arg1.GetFst<Arc>());
  const Fst<Arc> &ifst2 = *(args->arg2.GetFst<Arc>());
  MutableFst<Arc> *ofst = args->arg5->GetMutableFst<Arc>();

  vector<pair<typename Arc::Label, typename Arc::Label> > parens(
      args->arg3.size());

  for (size_t i = 0; i < parens.size(); ++i) {
    parens[i].first = args->arg3[i].first;
    parens[i].second = args->arg3[i].second;
  }

  vector<typename Arc::Label> assignments(args->arg4.size());

  for (size_t i = 0; i < assignments.size(); ++i) {
    assignments[i] = args->arg4[i];
  }

  if (args->arg7) {
    Compose(ifst1, parens, assignments, ifst2, ofst, args->arg6);
  } else {
    Compose(ifst1, ifst2, parens, assignments, ofst, args->arg6);
  }
}

void MPdtCompose(const FstClass & ifst1,
                 const FstClass & ifst2,
                 const vector<pair<int64, int64> > &parens,
                 const vector<int64> &assignments,
                 MutableFstClass *ofst,
                 const MPdtComposeOptions &copts,
                 bool left_pdt);

// PDT EXPAND

struct MPdtExpandOptions {
  bool connect;
  bool keep_parentheses;

  MPdtExpandOptions(bool c = true, bool k = false)
      : connect(c), keep_parentheses(k) {}
};

typedef args::Package<const FstClass &,
                      const vector<pair<int64, int64> >&,
                      const vector<int64>&,
                      MutableFstClass *, MPdtExpandOptions> MPdtExpandArgs;

template<class Arc>
void MPdtExpand(MPdtExpandArgs *args) {
  const Fst<Arc> &fst = *(args->arg1.GetFst<Arc>());
  MutableFst<Arc> *ofst = args->arg4->GetMutableFst<Arc>();

  vector<pair<typename Arc::Label, typename Arc::Label> > parens(
      args->arg2.size());
  for (size_t i = 0; i < parens.size(); ++i) {
    parens[i].first = args->arg2[i].first;
    parens[i].second = args->arg2[i].second;
  }

  vector<typename Arc::Label> assignments(args->arg3.size());

  for (size_t i = 0; i < assignments.size(); ++i) {
    assignments[i] = args->arg3[i];
  }

  Expand(fst, parens, assignments, ofst,
         ExpandOptions<Arc>(args->arg5.connect,
                            args->arg5.keep_parentheses));
}

void MPdtExpand(const FstClass &ifst,
                const vector<pair<int64, int64> > &parens,
                const vector<int64> &assignments,
                MutableFstClass *ofst, const MPdtExpandOptions &opts);

void MPdtExpand(const FstClass &ifst,
                const vector<pair<int64, int64> > &parens,
                const vector<int64> &assignments,
                MutableFstClass *ofst, bool connect);

// PDT REVERSE

typedef args::Package<const FstClass &,
                      const vector<pair<int64, int64> >&,
                      vector<int64>&,
                      MutableFstClass *> MPdtReverseArgsInternal;

typedef args::WithReturnValue<vector<int64>,
                              MPdtReverseArgsInternal> MPdtReverseArgs;

template<class Arc>
void MPdtReverse(MPdtReverseArgs *args) {
  const Fst<Arc> &fst = *(args->args.arg1.GetFst<Arc>());
  MutableFst<Arc> *ofst = args->args.arg4->GetMutableFst<Arc>();

  vector<pair<typename Arc::Label, typename Arc::Label> > parens(
      args->args.arg2.size());

  for (size_t i = 0; i < parens.size(); ++i) {
    parens[i].first = args->args.arg2[i].first;
    parens[i].second = args->args.arg2[i].second;
  }

  vector<typename Arc::Label> assignments(args->args.arg3.size());

  for (size_t i = 0; i < assignments.size(); ++i) {
    assignments[i] = args->args.arg3[i];
  }

  Reverse(fst, parens, &assignments, ofst);

  // Stack assignments now reassigned
  for (size_t i = 0; i < assignments.size(); ++i) {
    args->retval.push_back(assignments[i]);
  }
}

void MPdtReverse(const FstClass &ifst,
                 const vector<pair<int64, int64> > &parens,
                 vector<int64> *assignments,
                 MutableFstClass *ofst);


// PRINT INFO

typedef args::Package<const FstClass &,
                      const vector<pair<int64, int64> > &,
                      const vector<int64>&> PrintMPdtInfoArgs;

template<class Arc>
void PrintMPdtInfo(PrintMPdtInfoArgs *args) {
  const Fst<Arc> &fst = *(args->arg1.GetFst<Arc>());
  vector<pair<typename Arc::Label, typename Arc::Label> > parens(
      args->arg2.size());
  for (size_t i = 0; i < parens.size(); ++i) {
    parens[i].first = args->arg2[i].first;
    parens[i].second = args->arg2[i].second;
  }
  vector<typename Arc::Label> assignments(args->arg3.size());
  for (size_t i = 0; i < assignments.size(); ++i) {
    assignments[i] = args->arg3[i];
  }
  MPdtInfo<Arc> mpdtinfo(fst, parens, assignments);
  mpdtinfo.Print();
}

void PrintMPdtInfo(const FstClass &ifst,
                   const vector<pair<int64, int64> > &parens,
                   const vector<int64> &assignments);

}  // namespace script
}  // namespace fst


#define REGISTER_FST_MPDT_OPERATIONS(ArcType)                                \
  REGISTER_FST_OPERATION(MPdtCompose, ArcType, MPdtComposeArgs);             \
  REGISTER_FST_OPERATION(MPdtExpand, ArcType, MPdtExpandArgs);               \
  REGISTER_FST_OPERATION(MPdtReverse, ArcType, MPdtReverseArgs);             \
  REGISTER_FST_OPERATION(PrintMPdtInfo, ArcType, PrintMPdtInfoArgs)
#endif  // FST_EXTENSIONS_MPDT_MPDTSCRIPT_H_
