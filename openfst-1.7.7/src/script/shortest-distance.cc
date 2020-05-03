// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#include <fst/script/shortest-distance.h>

#include <fst/script/script-impl.h>

namespace fst {
namespace script {

void ShortestDistance(const FstClass &fst, std::vector<WeightClass> *distance,
                      const ShortestDistanceOptions &opts) {
  ShortestDistanceArgs1 args(fst, distance, opts);
  Apply<Operation<ShortestDistanceArgs1>>("ShortestDistance", fst.ArcType(),
                                          &args);
}

void ShortestDistance(const FstClass &ifst, std::vector<WeightClass> *distance,
                      bool reverse, double delta) {
  ShortestDistanceArgs2 args(ifst, distance, reverse, delta);
  Apply<Operation<ShortestDistanceArgs2>>("ShortestDistance", ifst.ArcType(),
                                          &args);
}

REGISTER_FST_OPERATION_3ARCS(ShortestDistance, ShortestDistanceArgs1);
REGISTER_FST_OPERATION_3ARCS(ShortestDistance, ShortestDistanceArgs2);

}  // namespace script
}  // namespace fst
