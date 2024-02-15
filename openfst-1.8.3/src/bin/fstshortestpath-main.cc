// Copyright 2005-2024 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the 'License');
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an 'AS IS' BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Find shortest path(s) in an FST.

#include <cstring>
#include <memory>
#include <string>

#include <fst/flags.h>
#include <fst/log.h>
#include <fst/queue.h>
#include <fst/script/fst-class.h>
#include <fst/script/getters.h>
#include <fst/script/shortest-path.h>
#include <fst/script/weight-class.h>

DECLARE_double(delta);
DECLARE_int32(nshortest);
DECLARE_int64(nstate);
DECLARE_string(queue_type);
DECLARE_bool(unique);
DECLARE_string(weight);

int fstshortestpath_main(int argc, char **argv) {
  namespace s = fst::script;
  using fst::QueueType;
  using fst::script::FstClass;
  using fst::script::VectorFstClass;
  using fst::script::WeightClass;

  std::string usage = "Finds shortest path(s) in an FST.\n\n  Usage: ";
  usage += argv[0];
  usage += " [in.fst [out.fst]]\n";

  SET_FLAGS(usage.c_str(), &argc, &argv, true);
  if (argc > 3) {
    ShowUsage();
    return 1;
  }

  const std::string in_name =
      (argc > 1 && strcmp(argv[1], "-") != 0) ? argv[1] : "";
  const std::string out_name =
      (argc > 2 && strcmp(argv[2], "-") != 0) ? argv[2] : "";

  std::unique_ptr<FstClass> ifst(FstClass::Read(in_name));
  if (!ifst) return 1;

  const auto weight_threshold =
      FST_FLAGS_weight.empty()
          ? WeightClass::Zero(ifst->WeightType())
          : WeightClass(ifst->WeightType(), FST_FLAGS_weight);

  VectorFstClass ofst(ifst->ArcType());

  QueueType queue_type;
  if (!s::GetQueueType(FST_FLAGS_queue_type, &queue_type)) {
    LOG(ERROR) << "Unknown or unsupported queue type: "
               << FST_FLAGS_queue_type;
    return 1;
  }

  const s::ShortestPathOptions opts(
      queue_type, FST_FLAGS_nshortest, FST_FLAGS_unique,
      FST_FLAGS_delta, weight_threshold,
      FST_FLAGS_nstate);

  s::ShortestPath(*ifst, &ofst, opts);

  return !ofst.Write(out_name);
}
