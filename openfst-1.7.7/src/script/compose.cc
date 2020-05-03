// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#include <fst/script/compose.h>

#include <fst/script/script-impl.h>

namespace fst {
namespace script {

void Compose(const FstClass &ifst1, const FstClass &ifst2,
             MutableFstClass *ofst, const ComposeOptions &opts) {
  if (!internal::ArcTypesMatch(ifst1, ifst2, "Compose") ||
      !internal::ArcTypesMatch(*ofst, ifst1, "Compose")) {
    ofst->SetProperties(kError, kError);
    return;
  }
  ComposeArgs args(ifst1, ifst2, ofst, opts);
  Apply<Operation<ComposeArgs>>("Compose", ifst1.ArcType(), &args);
}

REGISTER_FST_OPERATION_3ARCS(Compose, ComposeArgs);

}  // namespace script
}  // namespace fst
