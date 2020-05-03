// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#include <fst/script/encode.h>

#include <fst/script/script-impl.h>

namespace fst {
namespace script {

void Encode(MutableFstClass *fst, EncodeMapperClass *mapper) {
  if (!internal::ArcTypesMatch(*fst, *mapper, "Encode")) {
    fst->SetProperties(kError, kError);
    return;
  }
  EncodeArgs args(fst, mapper);
  Apply<Operation<EncodeArgs>>("Encode", fst->ArcType(), &args);
}

REGISTER_FST_OPERATION_3ARCS(Encode, EncodeArgs);

}  // namespace script
}  // namespace fst
