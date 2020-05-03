// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#include <fst/script/reverse.h>

#include <fst/script/script-impl.h>

namespace fst {
namespace script {

void Reverse(const FstClass &ifst, MutableFstClass *ofst,
             bool require_superinitial) {
  if (!internal::ArcTypesMatch(ifst, *ofst, "Reverse")) {
    ofst->SetProperties(kError, kError);
    return;
  }
  ReverseArgs args(ifst, ofst, require_superinitial);
  Apply<Operation<ReverseArgs>>("Reverse", ifst.ArcType(), &args);
}

REGISTER_FST_OPERATION_3ARCS(Reverse, ReverseArgs);

}  // namespace script
}  // namespace fst
