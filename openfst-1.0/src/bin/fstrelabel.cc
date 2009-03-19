// fstrelabel.cc

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Author: johans@google.com (Johan Schalkwyk)
//
// \file
// Relabel input or output space of Fst
//

#include "./relabel-main.h"

namespace fst {

// Register templated main for common arcs types.
REGISTER_FST_MAIN(RelabelMain, StdArc);
REGISTER_FST_MAIN(RelabelMain, LogArc);

}  // namespace fst


int main(int argc, char **argv) {
  string usage = "Relabel the input and/or the output labels of the Fst.\n";
  usage += " Usage: ";
  usage += argv[0];
  usage += " [in.fst [out.fst]]\n";
  usage += " Using SymbolTables flags:\n";
  usage += "  -relabel_isymbols isyms.txt\n";
  usage += "  -relabel_osymbols osyms.txt\n";
  usage += " Using numeric labels flags:\n";
  usage += "  -relabel_ipairs   ipairs.txt\n";
  usage += "  -relabel_opairs   opairs.txts\n";

  std::set_new_handler(FailedNewHandler);
  SetFlags(usage.c_str(), &argc, &argv, true);
  if (argc > 3) {
    ShowUsage();
    return 1;
  }

  // Invokes RelabelMain<Arc> where arc type is determined from argv[1].
  return CALL_FST_MAIN(RelabelMain, argc, argv);
}
