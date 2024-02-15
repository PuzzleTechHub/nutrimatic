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
// Two DFAs are equivalent iff their exit status is zero.

#include <cstring>
#include <memory>
#include <string>

#include <fst/flags.h>
#include <fst/log.h>
#include <fst/randgen.h>
#include <fst/script/equivalent.h>
#include <fst/script/fst-class.h>
#include <fst/script/getters.h>
#include <fst/script/randequivalent.h>
#include <fst/script/script-impl.h>

DECLARE_double(delta);
DECLARE_bool(random);
DECLARE_int32(max_length);
DECLARE_int32(npath);
DECLARE_uint64(seed);
DECLARE_string(select);

int fstequivalent_main(int argc, char **argv) {
  namespace s = fst::script;
  using fst::RandGenOptions;
  using fst::script::FstClass;

  std::string usage =
      "Two DFAs are equivalent iff the exit status is zero.\n\n"
      "  Usage: ";
  usage += argv[0];
  usage += " in1.fst in2.fst\n";

  SET_FLAGS(usage.c_str(), &argc, &argv, true);
  if (argc != 3) {
    ShowUsage();
    return 1;
  }

  const std::string in1_name = strcmp(argv[1], "-") == 0 ? "" : argv[1];
  const std::string in2_name = strcmp(argv[2], "-") == 0 ? "" : argv[2];

  if (in1_name.empty() && in2_name.empty()) {
    LOG(ERROR) << argv[0] << ": Can't take both inputs from standard input";
    return 1;
  }

  std::unique_ptr<FstClass> ifst1(FstClass::Read(in1_name));
  if (!ifst1) return 1;

  std::unique_ptr<FstClass> ifst2(FstClass::Read(in2_name));
  if (!ifst2) return 1;

  bool result;
  if (FST_FLAGS_random) {
    s::RandArcSelection ras;
    if (!s::GetRandArcSelection(FST_FLAGS_select, &ras)) {
      LOG(ERROR) << argv[0] << ": Unknown or unsupported select type "
                 << FST_FLAGS_select;
      return 1;
    }
    const RandGenOptions<s::RandArcSelection> opts(
        ras, FST_FLAGS_max_length);
    const auto seed = s::GetSeed(FST_FLAGS_seed);
    VLOG(1) << argv[0] << ": Seed = " << seed;
    result = s::RandEquivalent(*ifst1, *ifst2, FST_FLAGS_npath, opts,
                               FST_FLAGS_delta, seed);
  } else {
    result = s::Equivalent(*ifst1, *ifst2, FST_FLAGS_delta);
  }

  if (!result) VLOG(1) << "FSTs are not equivalent";

  return result ? 0 : 2;
}
