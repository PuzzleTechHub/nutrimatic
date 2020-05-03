// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#include <fst/script/union.h>

#include <fst/script/script-impl.h>

namespace fst {
namespace script {

void Union(MutableFstClass *fst1, const FstClass &fst2) {
  if (!internal::ArcTypesMatch(*fst1, fst2, "Union")) {
    fst1->SetProperties(kError, kError);
    return;
  }
  UnionArgs1 args(fst1, fst2);
  Apply<Operation<UnionArgs1>>("Union", fst1->ArcType(), &args);
}

void Union(MutableFstClass *fst1, const std::vector<const FstClass *> &fsts2) {
  for (const auto *fst2 : fsts2) {
    if (!internal::ArcTypesMatch(*fst1, *fst2, "Union")) {
      fst1->SetProperties(kError, kError);
      return;
    }
  }
  UnionArgs2 args(fst1, fsts2);
  Apply<Operation<UnionArgs2>>("Union", fst1->ArcType(), &args);
}

REGISTER_FST_OPERATION_3ARCS(Union, UnionArgs1);
REGISTER_FST_OPERATION_3ARCS(Union, UnionArgs2);

}  // namespace script
}  // namespace fst
