// compose-main.h

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
// Composes two FSTs. Includes helper function for fstcompose.cc
// that templates the main on the arc type to support multiple and
// extensible arc types.
//

#ifndef FST_COMPOSE_MAIN_H__
#define FST_COMPOSE_MAIN_H__

#include <fst/compose.h>
#include <fst/main.h>
#include <fst/vector-fst.h>

DECLARE_bool(connect);
DECLARE_string(compose_filter);

namespace fst {

// Main function for fstcompose templated on the arc type.
// Call only with 'return CALL_FST_MAIN' in main().
template <class Arc>
int ComposeMain(int argc, char **argv, istream &strm,
                const FstReadOptions &opts) {
  Fst<Arc> *ifst1 = Fst<Arc>::Read(strm, opts);
  if (!ifst1) return 1;

  if ((opts.source == "standard input" && strcmp(argv[2], "-") == 0)
      || strcmp(argv[2], "") == 0) {
    LOG(ERROR) << argv[0] << ": Can't open file: " << argv[2];
    return 1;
  }
  Fst<Arc> *ifst2 = Fst<Arc>::Read(strcmp(argv[2], "-") != 0 ? argv[2] : "");
  if (!ifst2) return 1;

  VectorFst<Arc> ofst;
  if (FLAGS_compose_filter == "sequence") {
    ComposeFstOptions<Arc> copts;
    copts.gc_limit = 0;  // Cache only the last state for fastest copy.
    ofst = ComposeFst<Arc>(*ifst1, *ifst2, copts);
  } else if (FLAGS_compose_filter == "alt_sequence") {
    ComposeFstOptions<Arc, Matcher<Fst<Arc> >,
        AltSequenceComposeFilter<Arc> > copts;
    copts.gc_limit = 0;  // Cache only the last state for fastest copy.
    ofst = ComposeFst<Arc>(*ifst1, *ifst2, copts);
  } else if (FLAGS_compose_filter == "match") {
    ComposeFstOptions<Arc, Matcher<Fst<Arc> >,
        MatchComposeFilter<Arc> > copts;
    copts.gc_limit = 0;  // Cache only the last state for fastest copy.
    ofst = ComposeFst<Arc>(*ifst1, *ifst2, copts);
  } else {
    LOG(ERROR) << argv[0] << ": Bad filter type \""
               << FLAGS_compose_filter << "\"";
  }
  if (FLAGS_connect)
    Connect(&ofst);
  ofst.Write(argc > 3 ? argv[3] : "");

  return 0;
}

}  // namespace fst

#endif  // FST_COMPOSE_MAIN_H__
