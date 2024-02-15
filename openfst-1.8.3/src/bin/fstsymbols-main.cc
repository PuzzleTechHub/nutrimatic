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
// Performs operations (set, clear, relabel) on the symbols table attached to an
// input FST.

#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <fst/flags.h>
#include <fst/log.h>
#include <fst/symbol-table.h>
#include <fst/util.h>
#include <fst/script/fst-class.h>
#include <fst/script/verify.h>

DECLARE_string(isymbols);
DECLARE_string(osymbols);
DECLARE_bool(clear_isymbols);
DECLARE_bool(clear_osymbols);
DECLARE_string(relabel_ipairs);
DECLARE_string(relabel_opairs);
DECLARE_string(save_isymbols);
DECLARE_string(save_osymbols);
DECLARE_bool(verify);

int fstsymbols_main(int argc, char **argv) {
  namespace s = fst::script;
  using fst::ReadLabelPairs;
  using fst::SymbolTable;
  using fst::script::MutableFstClass;

  std::string usage =
      "Performs operations (set, clear, relabel) on the symbol"
      " tables attached to an FST.\n\n  Usage: ";
  usage += argv[0];
  usage += " [in.fst [out.fst]]\n";

  SET_FLAGS(usage.c_str(), &argc, &argv, true);
  if (argc > 3) {
    ShowUsage();
    return 1;
  }

  const std::string in_name =
      argc > 1 && strcmp(argv[1], "-") != 0 ? argv[1] : "";
  const std::string out_name =
      argc > 2 && strcmp(argv[2], "-") != 0 ? argv[2] : "";

  std::unique_ptr<MutableFstClass> fst(MutableFstClass::Read(in_name, true));
  if (!fst) return 1;

  if (!FST_FLAGS_save_isymbols.empty()) {
    const auto *isyms = fst->InputSymbols();
    if (isyms) {
      isyms->WriteText(FST_FLAGS_save_isymbols);
    } else {
      LOG(ERROR) << argv[0]
                 << ": Saving isymbols but there are no input symbols.";
    }
  }

  if (!FST_FLAGS_save_osymbols.empty()) {
    const auto *osyms = fst->OutputSymbols();
    if (osyms) {
      osyms->WriteText(FST_FLAGS_save_osymbols);
    } else {
      LOG(ERROR) << argv[0]
                 << ": Saving osymbols but there are no output symbols.";
    }
  }

  std::unique_ptr<SymbolTable> isyms;
  if (!FST_FLAGS_isymbols.empty()) {
    isyms.reset(
        SymbolTable::ReadText(FST_FLAGS_isymbols,
                              FST_FLAGS_fst_field_separator));
    fst->SetInputSymbols(isyms.get());
  } else if (FST_FLAGS_clear_isymbols) {
    fst->SetInputSymbols(nullptr);
  }
  std::unique_ptr<SymbolTable> osyms;
  if (!FST_FLAGS_osymbols.empty()) {
    osyms.reset(
        SymbolTable::ReadText(FST_FLAGS_osymbols,
                              FST_FLAGS_fst_field_separator));
    fst->SetOutputSymbols(osyms.get());
  } else if (FST_FLAGS_clear_osymbols) {
    fst->SetOutputSymbols(nullptr);
  }

  using Label = int64_t;
  if (!FST_FLAGS_relabel_ipairs.empty()) {
    std::vector<std::pair<Label, Label>> ipairs;
    ReadLabelPairs(FST_FLAGS_relabel_ipairs, &ipairs);
    std::unique_ptr<SymbolTable> isyms_relabel(
        RelabelSymbolTable(fst->InputSymbols(), ipairs));
    fst->SetInputSymbols(isyms_relabel.get());
  }
  if (!FST_FLAGS_relabel_opairs.empty()) {
    std::vector<std::pair<Label, Label>> opairs;
    ReadLabelPairs(FST_FLAGS_relabel_opairs, &opairs);
    std::unique_ptr<SymbolTable> osyms_relabel(
        RelabelSymbolTable(fst->OutputSymbols(), opairs));
    fst->SetOutputSymbols(osyms_relabel.get());
  }

  if (FST_FLAGS_verify && !s::Verify(*fst)) return 1;

  return !fst->Write(out_name);
}
