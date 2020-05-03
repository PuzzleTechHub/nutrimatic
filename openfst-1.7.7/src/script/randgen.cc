// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#include <fst/script/randgen.h>

#include <fst/script/script-impl.h>

namespace fst {
namespace script {

void RandGen(const FstClass &ifst, MutableFstClass *ofst,
             const RandGenOptions<RandArcSelection> &opts, uint64 seed) {
  if (!internal::ArcTypesMatch(ifst, *ofst, "RandGen")) {
    ofst->SetProperties(kError, kError);
    return;
  }
  RandGenArgs args(ifst, ofst, opts, seed);
  Apply<Operation<RandGenArgs>>("RandGen", ifst.ArcType(), &args);
}

REGISTER_FST_OPERATION_3ARCS(RandGen, RandGenArgs);

}  // namespace script
}  // namespace fst
