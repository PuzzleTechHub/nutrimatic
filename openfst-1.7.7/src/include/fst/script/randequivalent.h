// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#ifndef FST_SCRIPT_RANDEQUIVALENT_H_
#define FST_SCRIPT_RANDEQUIVALENT_H_

#include <tuple>

#include <fst/types.h>
#include <fst/randequivalent.h>
#include <fst/script/arg-packs.h>
#include <fst/script/fst-class.h>
#include <fst/script/script-impl.h>

namespace fst {
namespace script {

using RandEquivalentInnerArgs =
    std::tuple<const FstClass &, const FstClass &, int32,
               const RandGenOptions<RandArcSelection> &, float, uint64>;

using RandEquivalentArgs = WithReturnValue<bool, RandEquivalentInnerArgs>;

template <class Arc>
void RandEquivalent(RandEquivalentArgs *args) {
  const Fst<Arc> &fst1 = *std::get<0>(args->args).GetFst<Arc>();
  const Fst<Arc> &fst2 = *std::get<1>(args->args).GetFst<Arc>();
  const int32 npath = std::get<2>(args->args);
  const auto &opts = std::get<3>(args->args);
  const float delta = std::get<4>(args->args);
  const uint64 seed = std::get<5>(args->args);
  switch (opts.selector) {
    case UNIFORM_ARC_SELECTOR: {
      const UniformArcSelector<Arc> selector(seed);
      const RandGenOptions<UniformArcSelector<Arc>> ropts(selector,
                                                          opts.max_length);
      args->retval = RandEquivalent(fst1, fst2, npath, ropts, delta, seed);
      return;
    }
    case FAST_LOG_PROB_ARC_SELECTOR: {
      const FastLogProbArcSelector<Arc> selector(seed);
      const RandGenOptions<FastLogProbArcSelector<Arc>> ropts(selector,
                                                              opts.max_length);
      args->retval = RandEquivalent(fst1, fst2, npath, ropts, delta, seed);
      return;
    }
    case LOG_PROB_ARC_SELECTOR: {
      const LogProbArcSelector<Arc> selector(seed);
      const RandGenOptions<LogProbArcSelector<Arc>> ropts(selector,
                                                          opts.max_length);
      args->retval = RandEquivalent(fst1, fst2, npath, ropts, delta, seed);
      return;
    }
  }
}

bool RandEquivalent(const FstClass &fst1, const FstClass &fst2, int32 npath = 1,
                    const RandGenOptions<RandArcSelection> &opts =
                        RandGenOptions<RandArcSelection>(UNIFORM_ARC_SELECTOR),
                    float delta = kDelta, uint64 seed = std::random_device()());

}  // namespace script
}  // namespace fst

#endif  // FST_SCRIPT_RANDEQUIVALENT_H_
