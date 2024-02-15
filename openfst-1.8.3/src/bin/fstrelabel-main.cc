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
// Relabels input or output space of an FST.

#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <fst/flags.h>
#include <fst/symbol-table.h>
#include <fst/util.h>
#include <fst/script/fst-class.h>
#include <fst/script/relabel.h>

DECLARE_string(isymbols);
DECLARE_string(osymbols);
DECLARE_string(relabel_isymbols);
DECLARE_string(relabel_osymbols);
DECLARE_string(relabel_ipairs);
DECLARE_string(relabel_opairs);
DECLARE_string(unknown_isymbol);
DECLARE_string(unknown_osymbol);

int fstrelabel_main(int argc, char **argv) {
  namespace s = fst::script;
  using fst::ReadLabelPairs;
  using fst::SymbolTable;
  using fst::script::MutableFstClass;

  std::string usage =
      "Relabels the input and/or the output labels of the FST.\n\n"
      "  Usage: ";
  usage += argv[0];
  usage += " [in.fst [out.fst]]\n";
  usage += "\n Using SymbolTables flags:\n";
  usage += "  --relabel_isymbols isyms.map\n";
  usage += "  --relabel_osymbols osyms.map\n";
  usage += "\n Using numeric labels flags:\n";
  usage += "  --relabel_ipairs ipairs.txt\n";
  usage += "  --relabel_opairs opairs.txt\n";

  SET_FLAGS(usage.c_str(), &argc, &argv, true);
  if (argc > 3) {
    ShowUsage();
    return 1;
  }

  const std::string in_name =
      (argc > 1 && strcmp(argv[1], "-") != 0) ? argv[1] : "";
  const std::string out_name =
      (argc > 2 && strcmp(argv[2], "-") != 0) ? argv[2] : "";

  std::unique_ptr<MutableFstClass> fst(MutableFstClass::Read(in_name, true));
  if (!fst) return 1;

  if (!FST_FLAGS_relabel_isymbols.empty() ||
      !FST_FLAGS_relabel_osymbols.empty()) {
    bool attach_new_isymbols = (fst->InputSymbols() != nullptr);
    std::unique_ptr<const SymbolTable> old_isymbols(
        FST_FLAGS_isymbols.empty()
            ? nullptr
            : SymbolTable::ReadText(FST_FLAGS_isymbols,
                                    FST_FLAGS_fst_field_separator));
    const std::unique_ptr<const SymbolTable> relabel_isymbols(
        FST_FLAGS_relabel_isymbols.empty()
            ? nullptr
            : SymbolTable::ReadText(FST_FLAGS_relabel_isymbols,
                                    FST_FLAGS_fst_field_separator));
    bool attach_new_osymbols = (fst->OutputSymbols() != nullptr);
    std::unique_ptr<const SymbolTable> old_osymbols(
        FST_FLAGS_osymbols.empty()
            ? nullptr
            : SymbolTable::ReadText(FST_FLAGS_osymbols,
                                    FST_FLAGS_fst_field_separator));
    const std::unique_ptr<const SymbolTable> relabel_osymbols(
        FST_FLAGS_relabel_osymbols.empty()
            ? nullptr
            : SymbolTable::ReadText(FST_FLAGS_relabel_osymbols,
                                    FST_FLAGS_fst_field_separator));
    s::Relabel(fst.get(),
               old_isymbols ? old_isymbols.get() : fst->InputSymbols(),
               relabel_isymbols.get(), FST_FLAGS_unknown_isymbol,
               attach_new_isymbols,
               old_osymbols ? old_osymbols.get() : fst->OutputSymbols(),
               relabel_osymbols.get(), FST_FLAGS_unknown_osymbol,
               attach_new_osymbols);
  } else {
    // Reads in relabeling pairs.
    std::vector<std::pair<int64_t, int64_t>> ipairs;
    if (!FST_FLAGS_relabel_ipairs.empty() &&
        !ReadLabelPairs(FST_FLAGS_relabel_ipairs, &ipairs)) {
      return 1;
    }
    std::vector<std::pair<int64_t, int64_t>> opairs;
    if (!FST_FLAGS_relabel_opairs.empty() &&
        !ReadLabelPairs(FST_FLAGS_relabel_opairs, &opairs)) {
      return 1;
    }
    s::Relabel(fst.get(), ipairs, opairs);
  }

  return !fst->Write(out_name);
}
