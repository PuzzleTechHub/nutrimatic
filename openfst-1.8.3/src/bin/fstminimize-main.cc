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
// Minimizes a deterministic FST.

#include <cstring>
#include <memory>
#include <string>

#include <fst/flags.h>
#include <fst/log.h>
#include <fst/script/fst-class.h>
#include <fst/script/minimize.h>

DECLARE_double(delta);
DECLARE_bool(allow_nondet);

int fstminimize_main(int argc, char **argv) {
  namespace s = fst::script;
  using fst::script::MutableFstClass;
  using fst::script::VectorFstClass;

  std::string usage = "Minimizes a deterministic FST.\n\n  Usage: ";
  usage += argv[0];
  usage += " [in.fst [out1.fst [out2.fst]]]\n";

  SET_FLAGS(usage.c_str(), &argc, &argv, true);
  if (argc > 4) {
    ShowUsage();
    return 1;
  }

  const std::string in_name =
      (argc > 1 && strcmp(argv[1], "-") != 0) ? argv[1] : "";
  const std::string out1_name =
      (argc > 2 && strcmp(argv[2], "-") != 0) ? argv[2] : "";
  const std::string out2_name =
      (argc > 3 && strcmp(argv[3], "-") != 0) ? argv[3] : "";

  if (out1_name.empty() && out2_name.empty() && argc > 3) {
    LOG(ERROR) << argv[0] << ": Both outputs can't be standard output.";
    return 1;
  }

  std::unique_ptr<MutableFstClass> fst1(MutableFstClass::Read(in_name, true));
  if (!fst1) return 1;

  if (argc > 3) {
    std::unique_ptr<MutableFstClass> fst2(new VectorFstClass(fst1->ArcType()));
    s::Minimize(fst1.get(), fst2.get(), FST_FLAGS_delta,
                FST_FLAGS_allow_nondet);
    if (!fst2->Write(out2_name)) return 1;
  } else {
    s::Minimize(fst1.get(), nullptr, FST_FLAGS_delta,
                FST_FLAGS_allow_nondet);
  }

  return !fst1->Write(out1_name);
}
