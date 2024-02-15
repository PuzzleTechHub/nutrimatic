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

DEFINE_string(isymbols, "", "Input label symbol table");
DEFINE_string(osymbols, "", "Output label symbol table");
DEFINE_bool(clear_isymbols, false, "Clear input symbol table");
DEFINE_bool(clear_osymbols, false, "Clear output symbol table");
DEFINE_string(relabel_ipairs, "", "Input relabel pairs (numeric)");
DEFINE_string(relabel_opairs, "", "Output relabel pairs (numeric)");
DEFINE_string(save_isymbols, "", "Save fst file's input symbol table to file");
DEFINE_string(save_osymbols, "", "Save fst file's output symbol table to file");
DEFINE_bool(verify, false, "Verify fst properities before saving");

int fstsymbols_main(int argc, char **argv);

int main(int argc, char **argv) { return fstsymbols_main(argc, argv); }
