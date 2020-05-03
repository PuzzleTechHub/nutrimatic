// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#include <fst/script/shortest-path.h>

#include <fst/script/script-impl.h>

namespace fst {
namespace script {

void ShortestPath(const FstClass &ifst, MutableFstClass *ofst,
                  const ShortestPathOptions &opts) {
  if (!internal::ArcTypesMatch(ifst, *ofst, "ShortestPath")) {
    ofst->SetProperties(kError, kError);
    return;
  }
  ShortestPathArgs args(ifst, ofst, opts);
  Apply<Operation<ShortestPathArgs>>("ShortestPath", ifst.ArcType(), &args);
}

REGISTER_FST_OPERATION_3ARCS(ShortestPath, ShortestPathArgs);

}  // namespace script
}  // namespace fst
