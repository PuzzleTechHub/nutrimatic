// determinize-main.h

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
// Determinizes an FST. Includes helper function for fstdeterminize.cc
// that templates the main on the arc type to support multiple and
// extensible arc types.
//

#ifndef FST_DETERMINIZE_MAIN_H__
#define FST_DETERMINIZE_MAIN_H__

#include <fst/determinize.h>
#include <fst/vector-fst.h>
#include <fst/main.h>
#include <fst/text-io.h>

DECLARE_double(delta);
DECLARE_int64(nstate);
DECLARE_string(weight);
DECLARE_int64(subsequential_label);

namespace fst {

// Main function for fstdeterminize templated on the arc type.
// Call only with 'return CALL_FST_MAIN' in main().
template <class Arc>
int DeterminizeMain(int argc, char **argv, istream &strm,
             const FstReadOptions &iopts) {
  typedef typename Arc::Weight Weight;

  Fst<Arc> *ifst = Fst<Arc>::Read(strm, iopts);
  if (!ifst) return 1;

  VectorFst<Arc> ofst;
  DeterminizeOptions<Arc> opts;
  opts.weight_threshold = FLAGS_weight.empty() ? Weight::Zero() :
      StrToWeight<Weight>(FLAGS_weight, "FLAGS_weight", 0);
  opts.state_threshold = FLAGS_nstate;
  opts.delta = FLAGS_delta;
  opts.subsequential_label = FLAGS_subsequential_label;
  Determinize(*ifst, &ofst, opts);
  ofst.Write(argc > 2 ? argv[2] : "");

  return 0;
}

}  // namespace fst

#endif  // FST_DETERMINIZE_MAIN_H__
