// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#include <fst/script/isomorphic.h>

#include <fst/script/script-impl.h>

namespace fst {
namespace script {

bool Isomorphic(const FstClass &fst1, const FstClass &fst2, float delta) {
  if (!internal::ArcTypesMatch(fst1, fst2, "Isomorphic")) return false;
  IsomorphicInnerArgs iargs(fst1, fst2, delta);
  IsomorphicArgs args(iargs);
  Apply<Operation<IsomorphicArgs>>("Isomorphic", fst1.ArcType(), &args);
  return args.retval;
}

REGISTER_FST_OPERATION_3ARCS(Isomorphic, IsomorphicArgs);

}  // namespace script
}  // namespace fst
