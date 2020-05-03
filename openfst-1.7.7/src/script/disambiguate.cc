// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#include <fst/script/disambiguate.h>

#include <fst/script/script-impl.h>

namespace fst {
namespace script {

void Disambiguate(const FstClass &ifst, MutableFstClass *ofst,
                  const DisambiguateOptions &opts) {
  if (!internal::ArcTypesMatch(ifst, *ofst, "Disambiguate") ||
      !ofst->WeightTypesMatch(opts.weight_threshold, "Disambiguate")) {
    ofst->SetProperties(kError, kError);
    return;
  }
  DisambiguateArgs args(ifst, ofst, opts);
  Apply<Operation<DisambiguateArgs>>("Disambiguate", ifst.ArcType(), &args);
}

REGISTER_FST_OPERATION_3ARCS(Disambiguate, DisambiguateArgs);

}  // namespace script
}  // namespace fst
