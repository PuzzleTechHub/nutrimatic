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
#include <fst/weight.h>

DEFINE_double(delta, fst::kDelta, "Comparison/quantization delta");
DEFINE_string(weight, "", "Weight threshold");
DEFINE_int64(nstate, fst::kNoStateId, "State number threshold");
DEFINE_int64(subsequential_label, 0,
             "Input label of arc corresponding to residual final output when"
             " producing a subsequential transducer");
DEFINE_string(det_type, "functional",
              "Type of determinization: \"functional\", "
              "\"nonfunctional\", \"disambiguate\"");
DEFINE_bool(increment_subsequential_label, false,
            "Increment subsequential_label to obtain distinct labels for "
            " subsequential arcs at a given state");

int fstdeterminize_main(int argc, char **argv);

int main(int argc, char **argv) { return fstdeterminize_main(argc, argv); }
