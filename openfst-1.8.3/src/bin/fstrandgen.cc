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
#include <cstdint>
#include <limits>

#include <fst/flags.h>
#include <fst/script/getters.h>

DEFINE_int32(max_length, std::numeric_limits<int32_t>::max(),
             "Maximum path length");
DEFINE_int32(npath, 1, "Number of paths to generate");
DEFINE_uint64(seed, ::fst::script::kDefaultSeed, "Random seed");
DEFINE_string(select, "uniform",
              "Selection type: one of "
              " \"uniform\", \"log_prob\" (when appropriate),"
              " \"fast_log_prob\" (when appropriate)");
DEFINE_bool(weighted, false,
            "Output tree weighted by path count vs. unweighted paths");
DEFINE_bool(remove_total_weight, false,
            "Remove total weight when output weighted");

int fstrandgen_main(int argc, char **argv);

int main(int argc, char **argv) { return fstrandgen_main(argc, argv); }
