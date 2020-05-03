// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#include <fst/script/topsort.h>

#include <fst/script/script-impl.h>

namespace fst {
namespace script {

bool TopSort(MutableFstClass *fst) {
  TopSortArgs args(fst);
  Apply<Operation<TopSortArgs>>("TopSort", fst->ArcType(), &args);
  return args.retval;
}

REGISTER_FST_OPERATION_3ARCS(TopSort, TopSortArgs);

}  // namespace script
}  // namespace fst
