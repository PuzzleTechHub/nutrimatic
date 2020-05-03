// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#include <fst/script/rmepsilon.h>

#include <fst/script/script-impl.h>

namespace fst {
namespace script {

void RmEpsilon(MutableFstClass *fst, const RmEpsilonOptions &opts) {
  if (!fst->WeightTypesMatch(opts.weight_threshold, "RmEpsilon")) {
    fst->SetProperties(kError, kError);
    return;
  }
  RmEpsilonArgs args(fst, opts);
  Apply<Operation<RmEpsilonArgs>>("RmEpsilon", fst->ArcType(), &args);
}

REGISTER_FST_OPERATION_3ARCS(RmEpsilon, RmEpsilonArgs);

}  // namespace script
}  // namespace fst
