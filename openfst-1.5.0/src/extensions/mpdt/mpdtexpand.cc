// mpdtexpand.cc

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
// Author: riley@google.com (Michael Riley)
// Modified: jpr@google.com (Jake Ratkiewicz) to use FstClass
// Author: rws@google.com (Richard Sproat)
//
// \file
// Expands a (bounded-stack) MPDT as an FST.
//

#include <fst/extensions/mpdt/read_write_utils.h>
#include <fst/extensions/mpdt/mpdtscript.h>
#include <fst/util.h>

DEFINE_string(mpdt_parentheses, "",
              "MPDT parenthesis label pairs with assignments.");
DEFINE_bool(connect, true, "Trim output");
DEFINE_bool(keep_parentheses, false, "Keep PDT parentheses in result.");


int main(int argc, char **argv) {
  namespace s = fst::script;

  string usage = "Expand a (bounded-stack) MPDT as an FST.\n\n  Usage: ";
  usage += argv[0];
  usage += " in.pdt [out.fst]\n";

  std::set_new_handler(FailedNewHandler);
  SET_FLAGS(usage.c_str(), &argc, &argv, true);
  if (argc > 3) {
    ShowUsage();
    return 1;
  }

  string in_name = (argc > 1 && (strcmp(argv[1], "-") != 0)) ? argv[1] : "";
  string out_name = argc > 2 ? argv[2] : "";

  s::FstClass *ifst = s::FstClass::Read(in_name);
  if (!ifst) return 1;

  if (FLAGS_mpdt_parentheses.empty()) {
    LOG(ERROR) << argv[0] << ": No MPDT parenthesis label pairs provided";
    return 1;
  }

  vector<pair<int64, int64> > parens;
  vector<int64> assignments;
  fst::ReadLabelTriples(FLAGS_mpdt_parentheses, &parens, &assignments,
                            false);

  s::VectorFstClass ofst(ifst->ArcType());
  s::MPdtExpand(*ifst, parens, assignments, &ofst,
                s::MPdtExpandOptions(FLAGS_connect, FLAGS_keep_parentheses));

  ofst.Write(out_name);

  return 0;
}
