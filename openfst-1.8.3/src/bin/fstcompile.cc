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

DEFINE_bool(acceptor, false, "Input in acceptor format");
DEFINE_string(arc_type, "standard", "Output arc type");
DEFINE_string(fst_type, "vector", "Output FST type");
DEFINE_string(isymbols, "", "Input label symbol table");
DEFINE_string(osymbols, "", "Output label symbol table");
DEFINE_string(ssymbols, "", "State label symbol table");
DEFINE_bool(keep_isymbols, false, "Store input label symbol table with FST");
DEFINE_bool(keep_osymbols, false, "Store output label symbol table with FST");
DEFINE_bool(keep_state_numbering, false, "Do not renumber input states");

int fstcompile_main(int argc, char **argv);

int main(int argc, char **argv) { return fstcompile_main(argc, argv); }
