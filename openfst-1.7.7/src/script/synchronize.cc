// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#include <fst/script/synchronize.h>

#include <fst/script/script-impl.h>

namespace fst {
namespace script {

void Synchronize(const FstClass &ifst, MutableFstClass *ofst) {
  if (!internal::ArcTypesMatch(ifst, *ofst, "Synchronize")) {
    ofst->SetProperties(kError, kError);
    return;
  }
  SynchronizeArgs args(ifst, ofst);
  Apply<Operation<SynchronizeArgs>>("Synchronize", ifst.ArcType(), &args);
}

REGISTER_FST_OPERATION_3ARCS(Synchronize, SynchronizeArgs);

}  // namespace script
}  // namespace fst
