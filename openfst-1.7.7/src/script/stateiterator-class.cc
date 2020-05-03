// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#include <fst/script/stateiterator-class.h>

#include <fst/script/script-impl.h>

namespace fst {
namespace script {

StateIteratorClass::StateIteratorClass(const FstClass &fst) : impl_(nullptr) {
  InitStateIteratorClassArgs args(fst, this);
  Apply<Operation<InitStateIteratorClassArgs>>("InitStateIteratorClass",
                                               fst.ArcType(), &args);
}

REGISTER_FST_OPERATION_3ARCS(InitStateIteratorClass,
                             InitStateIteratorClassArgs);

}  // namespace script
}  // namespace fst
