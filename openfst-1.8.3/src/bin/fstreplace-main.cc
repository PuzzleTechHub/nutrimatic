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
// Performs the dynamic replacement of arcs in one FST with another FST,
// allowing for the definition of FSTs analogous to RTNs.

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <fst/flags.h>
#include <fst/log.h>
#include <fst/replace-util.h>
#include <fst/script/fst-class.h>
#include <fst/script/getters.h>
#include <fst/script/replace.h>
#include <fst/script/script-impl.h>

DECLARE_string(call_arc_labeling);
DECLARE_string(return_arc_labeling);
DECLARE_int64(return_label);
DECLARE_bool(epsilon_on_replace);

int fstreplace_main(int argc, char **argv) {
  namespace s = fst::script;
  using fst::ReplaceLabelType;
  using fst::script::FstClass;
  using fst::script::VectorFstClass;

  std::string usage =
      "Recursively replaces FST arcs with other FST(s).\n\n"
      "  Usage: ";
  usage += argv[0];
  usage += " root.fst rootlabel [rule1.fst label1 ...] [out.fst]\n";

  SET_FLAGS(usage.c_str(), &argc, &argv, true);
  if (argc < 4) {
    ShowUsage();
    return 1;
  }

  const std::string out_name = argc % 2 == 0 ? argv[argc - 1] : "";

  std::vector<std::pair<int64_t, std::unique_ptr<const FstClass>>> pairs;
  for (auto i = 1; i < argc - 1; i += 2) {
    std::unique_ptr<const FstClass> ifst(FstClass::Read(argv[i]));
    if (!ifst) return 1;
    // Note that if the root label is beyond the range of the underlying FST's
    // labels, truncation will occur.
    const auto label = atoll(argv[i + 1]);
    pairs.emplace_back(label, std::move(ifst));
  }

  ReplaceLabelType call_label_type;
  if (!s::GetReplaceLabelType(FST_FLAGS_call_arc_labeling,
                              FST_FLAGS_epsilon_on_replace,
                              &call_label_type)) {
    LOG(ERROR) << argv[0] << ": Unknown or unsupported call arc replace "
               << "label type: " << FST_FLAGS_call_arc_labeling;
  }
  ReplaceLabelType return_label_type;
  if (!s::GetReplaceLabelType(FST_FLAGS_return_arc_labeling,
                              FST_FLAGS_epsilon_on_replace,
                              &return_label_type)) {
    LOG(ERROR) << argv[0] << ": Unknown or unsupported return arc replace "
               << "label type: " << FST_FLAGS_return_arc_labeling;
  }
  if (pairs.empty()) {
    LOG(ERROR) << argv[0] << "At least one replace pair must be provided.";
    return 1;
  }
  const auto root = pairs.front().first;
  const s::ReplaceOptions opts(root, call_label_type, return_label_type,
                               FST_FLAGS_return_label);

  VectorFstClass ofst(pairs.back().second->ArcType());
  s::Replace(s::BorrowPairs(pairs), &ofst, opts);

  return !ofst.Write(out_name);
}
