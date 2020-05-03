// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#include <fst/script/print.h>

#include <fst/script/script-impl.h>

namespace fst {
namespace script {

void Print(const FstClass &fst, std::ostream &ostrm, const std::string &dest,
           const SymbolTable *isyms, const SymbolTable *osyms,
           const SymbolTable *ssyms, bool accept, bool show_weight_one,
           const std::string &missing_sym) {
  const auto sep = FLAGS_fst_field_separator.substr(0, 1);
  PrintArgs args(fst, isyms, osyms, ssyms, accept, show_weight_one, ostrm, dest,
                 sep, missing_sym);
  Apply<Operation<PrintArgs>>("Print", fst.ArcType(), &args);
}

// TODO(kbg,2019-09-01): Deprecated.
void PrintFst(const FstClass &fst, std::ostream &ostrm, const std::string &dest,
              const SymbolTable *isyms, const SymbolTable *osyms,
              const SymbolTable *ssyms, bool accept, bool show_weight_one,
              const std::string &missing_sym) {
  Print(fst, ostrm, dest, isyms, osyms, ssyms, accept, show_weight_one,
        missing_sym);
}

REGISTER_FST_OPERATION_3ARCS(Print, PrintArgs);

}  // namespace script
}  // namespace fst
