
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

// Definitions of 'scriptable' versions of mpdt operations, that is,
// those that can be called with FstClass-type arguments.

// See comments in nlp/fst/script/script-impl.h for how the registration
// mechanism allows these to work with various arc types.

#include <vector>
using std::vector;
#include <utility>
using std::pair; using std::make_pair;


#include <fst/extensions/mpdt/compose.h>
#include <fst/extensions/mpdt/expand.h>
#include <fst/extensions/mpdt/mpdtscript.h>
#include <fst/extensions/mpdt/reverse.h>
#include <fst/script/script-impl.h>

namespace fst {
namespace script {

void MPdtCompose(const FstClass &ifst1,
                 const FstClass &ifst2,
                 const vector<pair<int64, int64> > &parens,
                 const vector<int64> &assignments,
                 MutableFstClass *ofst,
                 const MPdtComposeOptions &copts,
                 bool left_pdt) {
  if (!ArcTypesMatch(ifst1, ifst2, "MPdtCompose") ||
      !ArcTypesMatch(ifst1, *ofst, "MPdtCompose")) return;

  MPdtComposeArgs args(ifst1, ifst2, parens, assignments, ofst, copts,
                       left_pdt);

  Apply<Operation<MPdtComposeArgs> >("MPdtCompose", ifst1.ArcType(), &args);
}

void MPdtExpand(const FstClass &ifst,
                const vector<pair<int64, int64> > &parens,
                const vector<int64> &assignments,
                MutableFstClass *ofst, const MPdtExpandOptions &opts) {
  MPdtExpandArgs args(ifst, parens, assignments, ofst, opts);

  Apply<Operation<MPdtExpandArgs> >("MPdtExpand", ifst.ArcType(), &args);
}

void MPdtExpand(const FstClass &ifst,
                const vector<pair<int64, int64> > &parens,
                const vector<int64> &assignments,
                MutableFstClass *ofst, bool connect) {
  MPdtExpand(ifst, parens, assignments, ofst, MPdtExpandOptions(connect));
}

void MPdtReverse(const FstClass &ifst,
                 const vector<pair<int64, int64> > &parens,
                 vector<int64> *assignments,
                 MutableFstClass *ofst) {
  MPdtReverseArgsInternal args_internal(ifst, parens, *assignments, ofst);
  MPdtReverseArgs args(args_internal);
  Apply<Operation<MPdtReverseArgs> >("MPdtReverse", ifst.ArcType(), &args);
  // Stack assignments now reassigned
  for (size_t i = 0; i < assignments->size(); ++i) {
    (*assignments)[i] = args.retval[i];
  }
}

void PrintMPdtInfo(const FstClass &ifst,
                   const vector<pair<int64, int64> > &parens,
                   const vector<int64> &assignments) {
  PrintMPdtInfoArgs args(ifst, parens, assignments);
  Apply<Operation<PrintMPdtInfoArgs> >("PrintMPdtInfo", ifst.ArcType(), &args);
}

// Register operations for common arc types.

REGISTER_FST_MPDT_OPERATIONS(StdArc);
REGISTER_FST_MPDT_OPERATIONS(LogArc);
REGISTER_FST_MPDT_OPERATIONS(Log64Arc);

}  // namespace script
}  // namespace fst
