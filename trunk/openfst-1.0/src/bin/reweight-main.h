// reweight-main.h

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
// Reweights the paths in an FST. Includes helper function for
// fstreweight.cc that templates the main on the arc type to support
// multiple and extensible arc types.
//

#ifndef FST_REWEIGHT_MAIN_H__
#define FST_REWEIGHT_MAIN_H__

#include <vector>
#include <fst/reweight.h>
#include <fst/main.h>
#include <fst/text-io.h>
#include <fst/vector-fst.h>

DECLARE_bool(to_final);

namespace fst {

// Main function for fstreweight templated on the arc type.
// Call only with 'return CALL_FST_MAIN' in main().
template <class Arc>
int ReweightMain(int argc, char **argv, istream &strm,
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

  vector<Weight> potential;
  ReadPotentials(argv[2], &potential);

  ReweightType reweight_type = FLAGS_to_final ? REWEIGHT_TO_FINAL :
      REWEIGHT_TO_INITIAL;

  Reweight(ofst, potential, reweight_type);
  ofst->Write(argc > 3 ? argv[3] : "");

  return 0;
}

}  // namespace fst

#endif  // FST_REWEIGHT_MAIN_H__
