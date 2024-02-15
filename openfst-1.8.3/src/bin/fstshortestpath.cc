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

#include <fst/flags.h>
#include <fst/fst.h>
#include <fst/shortest-distance.h>

DEFINE_double(delta, fst::kShortestDelta, "Comparison/quantization delta");
DEFINE_int32(nshortest, 1, "Return N-shortest paths");
DEFINE_int64(nstate, fst::kNoStateId, "State number threshold");
DEFINE_string(queue_type, "auto",
              "Queue type: one of \"auto\", "
              "\"fifo\", \"lifo\", \"shortest\', \"state\", \"top\"");
DEFINE_bool(unique, false, "Return unique strings");
DEFINE_string(weight, "", "Weight threshold");

int fstshortestpath_main(int argc, char **argv);

int main(int argc, char **argv) { return fstshortestpath_main(argc, argv); }
