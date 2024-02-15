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

DEFINE_bool(acceptor, false, "Input in acceptor format?");
DEFINE_string(isymbols, "", "Input label symbol table");
DEFINE_string(osymbols, "", "Output label symbol table");
DEFINE_string(ssymbols, "", "State label symbol table");
DEFINE_bool(numeric, false, "Print numeric labels?");
DEFINE_string(save_isymbols, "", "Save input symbol table to file");
DEFINE_string(save_osymbols, "", "Save output symbol table to file");
DEFINE_bool(show_weight_one, false,
            "Print/draw arc weights and final weights equal to semiring One?");
DEFINE_string(missing_symbol, "",
              "Symbol to print when lookup fails (default raises error)");

int fstprint_main(int argc, char **argv);

int main(int argc, char **argv) { return fstprint_main(argc, argv); }
