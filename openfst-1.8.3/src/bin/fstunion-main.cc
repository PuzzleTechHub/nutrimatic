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
// Creates the union of two FSTs.

#include <cstring>
#include <memory>
#include <string>

#include <fst/flags.h>
#include <fst/log.h>
#include <fst/script/fst-class.h>
#include <fst/script/union.h>

int fstunion_main(int argc, char **argv) {
  namespace s = fst::script;
  using fst::script::FstClass;
  using fst::script::MutableFstClass;

  std::string usage = "Creates the union of two FSTs.\n\n  Usage: ";
  usage += argv[0];
  usage += " in1.fst in2.fst [out.fst]\n";

  SET_FLAGS(usage.c_str(), &argc, &argv, true);
  if (argc < 3 || argc > 4) {
    ShowUsage();
    return 1;
  }

  const std::string in1_name = strcmp(argv[1], "-") != 0 ? argv[1] : "";
  const std::string in2_name = strcmp(argv[2], "-") != 0 ? argv[2] : "";
  const std::string out_name =
      argc > 3 && strcmp(argv[3], "-") != 0 ? argv[3] : "";

  if (in1_name.empty() && in2_name.empty()) {
    LOG(ERROR) << argv[0] << ": Can't take both inputs from standard input";
    return 1;
  }

  std::unique_ptr<MutableFstClass> fst1(MutableFstClass::Read(in1_name, true));
  if (!fst1) return 1;

  std::unique_ptr<FstClass> fst2(FstClass::Read(in2_name));
  if (!fst2) return 1;

  s::Union(fst1.get(), *fst2);

  return !fst1->Write(out_name);
}
