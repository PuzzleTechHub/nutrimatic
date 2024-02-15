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

DEFINE_string(arc_filter, "any",
              "Arc filter: one of"
              " \"any\", \"epsilon\", \"iepsilon\", \"oepsilon\"; "
              "this only affects the counts of (co)accessible states, "
              "connected states, and (strongly) connected components");
DEFINE_string(info_type, "auto",
              "Info format: one of \"auto\", \"long\", \"short\", \"fast\".\n"
              "auto: Equivalent to \"long\" if the FST is an ExpandedFst,"
              " otherwise \"short\".\n"
              "long: Print all properties, computing if unknown;"
              " this may be slow for large FSTs.\n"
              "short: Print only type and SymbolTable info.\n"
              "fast: Print only info that is fast to obtain (by reading"
              " only the file header);" " this is more info than \"short\","
              " but less than \"long\".");
DEFINE_bool(test_properties, true,
            "Compute property values (if unknown to FST)");
DEFINE_bool(fst_verify, true, "Verify FST sanity");

int fstinfo_main(int argc, char **argv);

int main(int argc, char **argv) { return fstinfo_main(argc, argv); }
