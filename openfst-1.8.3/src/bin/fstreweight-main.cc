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
// Reweights an FST.

#include <memory>
#include <string>
#include <vector>

#include <fst/flags.h>
#include <fst/log.h>
#include <fst/reweight.h>
#include <fst/script/fst-class.h>
#include <fst/script/getters.h>
#include <fst/script/reweight.h>
#include <fst/script/text-io.h>
#include <fst/script/weight-class.h>

DECLARE_string(reweight_type);

int fstreweight_main(int argc, char **argv) {
  namespace s = fst::script;
  using fst::ReweightType;
  using fst::script::MutableFstClass;
  using fst::script::WeightClass;

  std::string usage = "Reweights an FST.\n\n  Usage: ";
  usage += argv[0];
  usage += " in.fst potential.txt [out.fst]\n";

  SET_FLAGS(usage.c_str(), &argc, &argv, true);
  if (argc < 3 || argc > 4) {
    ShowUsage();
    return 1;
  }

  const std::string in_name = argv[1];
  const std::string potentials_name = argv[2];
  const std::string out_name = argc > 3 ? argv[3] : "";

  std::unique_ptr<MutableFstClass> fst(MutableFstClass::Read(in_name, true));
  if (!fst) return 1;

  std::vector<WeightClass> potential;
  if (!s::ReadPotentials(fst->WeightType(), potentials_name, &potential)) {
    return 1;
  }

  ReweightType reweight_type;
  if (!s::GetReweightType(FST_FLAGS_reweight_type, &reweight_type)) {
    LOG(ERROR) << argv[0] << ": Unknown or unsupported reweight type: "
               << FST_FLAGS_reweight_type;
    return 1;
  }

  s::Reweight(fst.get(), potential, reweight_type);

  return !fst->Write(out_name);
}
