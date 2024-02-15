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
// Pushes weights and/or output labels in an FST toward the initial or final
// states.

#include <cstring>
#include <memory>
#include <string>

#include <fst/flags.h>
#include <fst/log.h>
#include <fst/reweight.h>
#include <fst/script/fst-class.h>
#include <fst/script/getters.h>
#include <fst/script/push.h>

DECLARE_double(delta);
DECLARE_bool(push_weights);
DECLARE_bool(push_labels);
DECLARE_bool(remove_total_weight);
DECLARE_bool(remove_common_affix);
DECLARE_string(reweight_type);

int fstpush_main(int argc, char **argv) {
  namespace s = fst::script;
  using fst::ReweightType;
  using fst::script::FstClass;
  using fst::script::VectorFstClass;

  std::string usage = "Pushes weights and/or olabels in an FST.\n\n  Usage: ";
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

  const auto flags = s::GetPushFlags(FST_FLAGS_push_weights,
                                     FST_FLAGS_push_labels,
                                     FST_FLAGS_remove_total_weight,
                                     FST_FLAGS_remove_common_affix);

  VectorFstClass ofst(ifst->ArcType());

  ReweightType reweight_type;
  if (!s::GetReweightType(FST_FLAGS_reweight_type, &reweight_type)) {
    LOG(ERROR) << argv[0] << ": Unknown or unsupported reweight type: "
               << FST_FLAGS_reweight_type;
    return 1;
  }

  s::Push(*ifst, &ofst, flags, reweight_type, FST_FLAGS_delta);

  return !ofst.Write(out_name);
}
