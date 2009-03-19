// push-main.h

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
// Pushes weights and/or output labels in an FST toward the initial or
// final states. Includes helper function for fstpush.cc that
// templates the main on the arc type to support multiple and
// extensible arc types.
//

#ifndef FST_PUSH_MAIN_H__
#define FST_PUSH_MAIN_H__

#include <fst/push.h>
#include <fst/main.h>
#include <fst/vector-fst.h>

DECLARE_double(delta);
DECLARE_bool(push_weights);
DECLARE_bool(push_labels);
DECLARE_bool(to_final);

namespace fst {

// Main function for fstpush templated on the arc type.
// Call only with 'return CALL_FST_MAIN' in main().
template <class Arc>
int PushMain(int argc, char **argv, istream &strm,
             const FstReadOptions &opts) {
  Fst<Arc> *ifst = Fst<Arc>::Read(strm, opts);
  if (!ifst) return 1;

  uint32 flags = 0;
  if (FLAGS_push_weights)
    flags |= kPushWeights;
  if (FLAGS_push_labels)
    flags |= kPushLabels;

  VectorFst<Arc> ofst;

  if (FLAGS_to_final)
    Push<Arc, REWEIGHT_TO_FINAL>(*ifst, &ofst, flags, FLAGS_delta);
  else
    Push<Arc, REWEIGHT_TO_INITIAL>(*ifst, &ofst, flags, FLAGS_delta);

  ofst.Write(argc > 2 ? argv[2] : "");

  return 0;
}

}  // namespace fst

#endif  // FST_PUSH_MAIN_H__
