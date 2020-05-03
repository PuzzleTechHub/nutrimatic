// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#include <fst/script/verify.h>

#include <fst/script/script-impl.h>

namespace fst {
namespace script {

bool Verify(const FstClass &fst) {
  VerifyArgs args(fst);
  Apply<Operation<VerifyArgs>>("Verify", fst.ArcType(), &args);
  return args.retval;
}

REGISTER_FST_OPERATION_3ARCS(Verify, VerifyArgs);

}  // namespace script
}  // namespace fst
