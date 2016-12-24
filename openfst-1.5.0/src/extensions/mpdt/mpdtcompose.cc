// mpdtcompose.cc

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
// Author: rws@google.com (Richard Sproat)
//
// \file
// Composes an MPDT and an FST.
//

#include <vector>
using std::vector;
#include <utility>
using std::pair; using std::make_pair;

#include <fst/util.h>
#include <fst/extensions/mpdt/read_write_utils.h>
#include <fst/extensions/mpdt/mpdtscript.h>
#include <fst/script/connect.h>

DEFINE_string(mpdt_parentheses, "",
              "MPDT parenthesis label pairs with assignments.");
DEFINE_bool(left_mpdt, true, "1st arg is MPDT (o.w. 2nd arg).");
DEFINE_bool(connect, true, "Trim output");
DEFINE_string(compose_filter, "paren",
              "Composition filter, one of: \"expand\", \"expand_paren\", "
              "\"paren\"");

int main(int argc, char **argv) {
  namespace s = fst::script;

  string usage = "Compose an MPDT and an FST.\n\n  Usage: ";
  usage += argv[0];
  usage += " in.pdt in.fst [out.mpdt]\n";
  usage += " in.fst in.pdt [out.mpdt]\n";

  std::set_new_handler(FailedNewHandler);
  SET_FLAGS(usage.c_str(), &argc, &argv, true);
  if (argc < 3 || argc > 4) {
    ShowUsage();
    return 1;
  }

  string in1_name = strcmp(argv[1], "-") == 0 ? "" : argv[1];
  string in2_name = strcmp(argv[2], "-") == 0 ? "" : argv[2];
  string out_name = argc > 3 ? argv[3] : "";

  if (in1_name.empty() && in2_name.empty()) {
    LOG(ERROR) << argv[0] << ": Can't take both inputs from standard input.";
    return 1;
  }

  s::FstClass *ifst1 = s::FstClass::Read(in1_name);
  if (!ifst1) return 1;
  s::FstClass *ifst2 = s::FstClass::Read(in2_name);
  if (!ifst2) return 1;

  if (FLAGS_mpdt_parentheses.empty()) {
    LOG(ERROR) << argv[0] << ": No MPDT parenthesis label pairs provided";
    return 1;
  }

  vector<pair<int64, int64> > parens;
  vector<int64> assignments;
  fst::ReadLabelTriples(FLAGS_mpdt_parentheses, &parens, &assignments,
                            false);

  s::VectorFstClass ofst(ifst1->ArcType());

  fst::PdtComposeFilter compose_filter;

  if (FLAGS_compose_filter == "expand") {
    compose_filter = fst::EXPAND_FILTER;
  } else if (FLAGS_compose_filter == "expand_paren") {
    compose_filter = fst::EXPAND_PAREN_FILTER;
  } else if (FLAGS_compose_filter == "paren") {
    compose_filter = fst::PAREN_FILTER;
  } else {
    LOG(ERROR) << argv[0] << "Unknown compose filter type: "
               << FLAGS_compose_filter;
    return 1;
  }

  fst::MPdtComposeOptions copts(false, compose_filter);

  s::MPdtCompose(*ifst1, *ifst2, parens, assignments, &ofst, copts,
                 FLAGS_left_mpdt);

  if (FLAGS_connect)
    s::Connect(&ofst);
  ofst.Write(out_name);

  return 0;
}
