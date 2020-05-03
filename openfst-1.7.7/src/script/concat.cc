// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#include <fst/script/concat.h>

#include <fst/script/script-impl.h>

namespace fst {
namespace script {

void Concat(MutableFstClass *fst1, const FstClass &fst2) {
  if (!internal::ArcTypesMatch(*fst1, fst2, "Concat")) {
    fst1->SetProperties(kError, kError);
    return;
  }
  ConcatArgs1 args(fst1, fst2);
  Apply<Operation<ConcatArgs1>>("Concat", fst1->ArcType(), &args);
}

void Concat(const FstClass &fst1, MutableFstClass *fst2) {
  if (!internal::ArcTypesMatch(fst1, *fst2, "Concat")) {
    fst2->SetProperties(kError, kError);
    return;
  }
  ConcatArgs2 args(fst1, fst2);
  Apply<Operation<ConcatArgs2>>("Concat", fst2->ArcType(), &args);
}

void Concat(const std::vector<FstClass *> &fsts1, MutableFstClass *fst2) {
  for (const auto *fst1 : fsts1) {
    if (!internal::ArcTypesMatch(*fst1, *fst2, "Concat")) {
      fst2->SetProperties(kError, kError);
      return;
    }
  }
  ConcatArgs3 args(fsts1, fst2);
  Apply<Operation<ConcatArgs3>>("Concat", fst2->ArcType(), &args);
}

REGISTER_FST_OPERATION_3ARCS(Concat, ConcatArgs1);
REGISTER_FST_OPERATION_3ARCS(Concat, ConcatArgs2);

}  // namespace script
}  // namespace fst
