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
// Determinizes an FST.

#include <cstring>
#include <memory>
#include <string>

#include <fst/flags.h>
#include <fst/log.h>
#include <fst/determinize.h>
#include <fst/script/determinize.h>
#include <fst/script/fst-class.h>
#include <fst/script/getters.h>
#include <fst/script/weight-class.h>

DECLARE_double(delta);
DECLARE_string(weight);
DECLARE_int64(nstate);
DECLARE_int64(subsequential_label);
DECLARE_string(det_type);
DECLARE_bool(increment_subsequential_label);

int fstdeterminize_main(int argc, char **argv) {
  namespace s = fst::script;
  using fst::DeterminizeType;
  using fst::script::FstClass;
  using fst::script::VectorFstClass;
  using fst::script::WeightClass;

  std::string usage = "Determinizes an FST.\n\n  Usage: ";
  usage += argv[0];
  usage += " [in.fst [out.fst]]\n";

  SET_FLAGS(usage.c_str(), &argc, &argv, true);
  if (argc > 3) {
    ShowUsage();
    return 1;
  }

  DeterminizeType det_type;
  if (!s::GetDeterminizeType(FST_FLAGS_det_type, &det_type)) {
    LOG(ERROR) << argv[0] << ": Unknown or unsupported determinization type: "
               << FST_FLAGS_det_type;
    return 1;
  }

  const std::string in_name =
      (argc > 1 && strcmp(argv[1], "-") != 0) ? argv[1] : "";
  const std::string out_name =
      (argc > 2 && strcmp(argv[2], "-") != 0) ? argv[2] : "";

  std::unique_ptr<FstClass> ifst(FstClass::Read(in_name));
  if (!ifst) return 1;

  VectorFstClass ofst(ifst->ArcType());

  const auto weight_threshold =
      FST_FLAGS_weight.empty()
          ? WeightClass::Zero(ifst->WeightType())
          : WeightClass(ifst->WeightType(), FST_FLAGS_weight);

  const s::DeterminizeOptions opts(
      FST_FLAGS_delta, weight_threshold, FST_FLAGS_nstate,
      FST_FLAGS_subsequential_label, det_type,
      FST_FLAGS_increment_subsequential_label);

  s::Determinize(*ifst, &ofst, opts);

  return !ofst.Write(out_name);
}
