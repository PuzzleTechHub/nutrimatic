// map-main.h

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
// Maps the paths in an FST. Includes helper function for
// fstmap.cc that templates the main on the arc type to support
// multiple and extensible arc types.
//

#ifndef FST_MAP_MAIN_H__
#define FST_MAP_MAIN_H__

#include <string>
#include <fst/map.h>
#include <fst/main.h>
#include <fst/text-io.h>
#include <fst/vector-fst.h>

DECLARE_double(delta);
DECLARE_string(map_type);
DECLARE_string(weight);

namespace fst {

// Main function for fstmap templated on the arc type.
// Call only with 'return CALL_FST_MAIN' in main().
template <class Arc>
int MapMain(int argc, char **argv, istream &strm,
             const FstReadOptions &opts) {
  typedef typename Arc::Weight Weight;

  Fst<Arc> *ifst = Fst<Arc>::Read(strm, opts);
  if (!ifst) return 1;

  MutableFst<Arc> *ofst = 0;
  if (ifst->Properties(kMutable, false)) {
    ofst = down_cast<MutableFst<Arc> *>(ifst);
  } else {
    ofst = new VectorFst<Arc>(*ifst);
    delete ifst;
  }

  if (FLAGS_map_type == "identity") {
    Map(ofst, IdentityMapper<Arc>());
  } else if (FLAGS_map_type == "invert") {
    Map(ofst, InvertWeightMapper<Arc>());
  } else if (FLAGS_map_type == "plus") {
    Weight w = FLAGS_weight.empty() ? Weight::Zero() :
        StrToWeight<Weight>(FLAGS_weight, "FLAGS_weight", 0);
    Map(ofst, PlusMapper<Arc>(w));
  } else if (FLAGS_map_type == "quantize") {
    Map(ofst, QuantizeMapper<Arc>(FLAGS_delta));
  } else if (FLAGS_map_type == "rmweight") {
    Map(ofst, RmWeightMapper<Arc>());
  } else if (FLAGS_map_type == "superfinal") {
    Map(ofst, SuperFinalMapper<Arc>());
  } else if (FLAGS_map_type == "times") {
    Weight w = FLAGS_weight.empty() ? Weight::One() :
        StrToWeight<Weight>(FLAGS_weight, "FLAGS_weight", 0);
    Map(ofst, TimesMapper<Arc>(w));
  } else {
    LOG(ERROR) << argv[0] << ": Unknown map type \""
               << FLAGS_map_type << "\"\n";
  }

    ofst->Write(argc > 2 ? argv[2] : "");

  return 0;
}

}  // namespace fst

#endif  // FST_MAP_MAIN_H__
