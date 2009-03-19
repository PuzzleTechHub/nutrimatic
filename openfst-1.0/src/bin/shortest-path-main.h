// shortest-path-main.h

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
// Find shortest path(s) in an FST.  Includes helper function for
// fstshortestpath.cc that templates the main on the arc type to
// support multiple and extensible arc types.
//

#ifndef FST_SHORTEST_PATH_MAIN_H__
#define FST_SHORTEST_PATH_MAIN_H__

#include <fst/shortest-path.h>
#include <fst/main.h>
#include <fst/text-io.h>
#include <fst/vector-fst.h>

DECLARE_double(delta);
DECLARE_int64(nshortest);
DECLARE_int64(nstate);
DECLARE_bool(unique);
DECLARE_string(weight);

namespace fst {

// Main function for fstshortestpath templated on the arc type.
// Call only with 'return CALL_FST_MAIN' in main().
template <class Arc>
int ShortestPathMain(int argc, char **argv, istream &strm,
             const FstReadOptions &ropts) {
  typedef typename Arc::StateId StateId;
  typedef typename Arc::Weight Weight;

  Fst<Arc> *ifst = Fst<Arc>::Read(strm, ropts);
  if (!ifst) return 1;

  Weight weight_threshold = FLAGS_weight.empty() ? Weight::Zero() :
      StrToWeight<Weight>(FLAGS_weight, "FLAGS_weight", 0);

  VectorFst<Arc> ofst;
  vector<Weight> distance;
  AnyArcFilter<Arc> arc_filter;
  AutoQueue<StateId> state_queue(*ifst, &distance, arc_filter);

  ShortestPathOptions< Arc, AutoQueue<StateId>, AnyArcFilter<Arc> >
      sopts(&state_queue, arc_filter, FLAGS_nshortest, FLAGS_unique,
            false, FLAGS_delta, false, weight_threshold, FLAGS_nstate);
  ShortestPath(*ifst, &ofst, &distance, sopts);
  ofst.Write(argc > 2 ? argv[2] : "");

  return 0;
}

}  // namespace fst

#endif  // FST_SHORTEST_PATH_MAIN_H__
