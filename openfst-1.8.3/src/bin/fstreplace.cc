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

DEFINE_string(call_arc_labeling, "input",
              "Which labels to make non-epsilon on the call arc: "
              "one of: \"input\" (default), \"output\", \"both\", \"neither\"");
DEFINE_string(return_arc_labeling, "neither",
              "Which labels to make non-epsilon on the return arc: "
              "one of: \"input\", \"output\", \"both\", \"neither\" (default)");
DEFINE_int64(return_label, 0, "Label to put on return arc");
DEFINE_bool(epsilon_on_replace, false, "Call/return arcs are epsilon arcs?");

int fstreplace_main(int argc, char **argv);

int main(int argc, char **argv) { return fstreplace_main(argc, argv); }
