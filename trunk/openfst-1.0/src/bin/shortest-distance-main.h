// shortest-distance-main.h

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
// Find shortest distance(s) in an FST.  Includes helper function for
// fstshortestdistance.cc that templates the main on the arc type to
// support multiple and extensible arc types.
//

#ifndef FST_SHORTEST_DISTANCE_MAIN_H__
#define FST_SHORTEST_DISTANCE_MAIN_H__

#include <string>
#include <vector>
#include <fst/shortest-distance.h>
#include <fst/main.h>
#include <fst/text-io.h>
#include <fst/vector-fst.h>

DECLARE_double(delta);
DECLARE_bool(reverse);

namespace fst {

// Main function for fstshortestdistance templated on the arc type.
// Call only with 'return CALL_FST_MAIN' in main().
template <class Arc>
int ShortestDistanceMain(int argc, char **argv, istream &strm,
             const FstReadOptions &opts) {
  typedef typename Arc::Weight Weight;

  Fst<Arc> *fst = Fst<Arc>::Read(strm, opts);

  if (!fst) return 1;

  vector<Weight> distance;
  ShortestDistance(*fst, &distance, FLAGS_reverse, FLAGS_delta);
  string ofilename;  // empty filename means 'standard output'
  if (argc == 3)
    ofilename = argv[2];
  WritePotentials(ofilename, distance);

  return 0;
}

}  // namespace fst

#endif  // FST_SHORTEST_DISTANCE_MAIN_H__
