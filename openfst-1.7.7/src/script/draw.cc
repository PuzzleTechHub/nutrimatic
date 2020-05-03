// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#include <fst/script/draw.h>

#include <fst/script/script-impl.h>

namespace fst {
namespace script {

void Draw(const FstClass &fst, const SymbolTable *isyms,
          const SymbolTable *osyms, const SymbolTable *ssyms, bool accep,
          const std::string &title, float width, float height, bool portrait,
          bool vertical, float ranksep, float nodesep, int fontsize,
          int precision, const std::string &float_format, bool show_weight_one,
          std::ostream &ostrm, const std::string &dest) {
  DrawArgs args(fst, isyms, osyms, ssyms, accep, title, width, height, portrait,
                vertical, ranksep, nodesep, fontsize, precision, float_format,
                show_weight_one, ostrm, dest);
  Apply<Operation<DrawArgs>>("Draw", fst.ArcType(), &args);
}

REGISTER_FST_OPERATION_3ARCS(Draw, DrawArgs);

}  // namespace script
}  // namespace fst
