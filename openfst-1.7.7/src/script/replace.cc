// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#include <fst/script/replace.h>

#include <fst/script/script-impl.h>

namespace fst {
namespace script {

void Replace(const std::vector<std::pair<int64, const FstClass *>> &pairs,
             MutableFstClass *ofst, const ReplaceOptions &opts) {
  for (const auto &pair : pairs) {
    if (!internal::ArcTypesMatch(*pair.second, *ofst, "Replace")) {
      ofst->SetProperties(kError, kError);
      return;
    }
  }
  ReplaceArgs args(pairs, ofst, opts);
  Apply<Operation<ReplaceArgs>>("Replace", ofst->ArcType(), &args);
}

REGISTER_FST_OPERATION_3ARCS(Replace, ReplaceArgs);

}  // namespace script
}  // namespace fst
