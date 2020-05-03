// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#include <fst/script/relabel.h>

#include <fst/script/script-impl.h>

namespace fst {
namespace script {

void Relabel(MutableFstClass *ofst, const SymbolTable *old_isyms,
             const SymbolTable *relabel_isyms,
             const std::string &unknown_isymbol, bool attach_new_isyms,
             const SymbolTable *old_osyms, const SymbolTable *relabel_osyms,
             const std::string &unknown_osymbol, bool attach_new_osyms) {
  RelabelArgs1 args(ofst, old_isyms, relabel_isyms, unknown_isymbol,
                    attach_new_isyms, old_osyms, relabel_osyms,
                    unknown_osymbol, attach_new_osyms);
  Apply<Operation<RelabelArgs1>>("Relabel", ofst->ArcType(), &args);
}

void Relabel(MutableFstClass *ofst,
             const std::vector<std::pair<int64, int64>> &ipairs,
             const std::vector<std::pair<int64, int64>> &opairs) {
  RelabelArgs2 args(ofst, ipairs, opairs);
  Apply<Operation<RelabelArgs2>>("Relabel", ofst->ArcType(), &args);
}

REGISTER_FST_OPERATION_3ARCS(Relabel, RelabelArgs1);
REGISTER_FST_OPERATION_3ARCS(Relabel, RelabelArgs2);

}  // namespace script
}  // namespace fst
