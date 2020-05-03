// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#include <fst/script/decode.h>

#include <fst/script/script-impl.h>

namespace fst {
namespace script {

void Decode(MutableFstClass *fst, const EncodeMapperClass &mapper) {
  if (!internal::ArcTypesMatch(*fst, mapper, "Decode")) {
    fst->SetProperties(kError, kError);
    return;
  }
  DecodeArgs args(fst, mapper);
  Apply<Operation<DecodeArgs>>("Decode", fst->ArcType(), &args);
}

REGISTER_FST_OPERATION_3ARCS(Decode, DecodeArgs);

}  // namespace script
}  // namespace fst
