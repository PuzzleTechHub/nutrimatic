// Copyright 2005-2024 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the 'License');
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an 'AS IS' BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Regression test for various FST algorithms.

#include <fst/test/algo_test.h>

#include <random>

#include <fst/flags.h>
#include <fst/log.h>
#include <fst/arc.h>
#include <fst/cache.h>
#include <fst/float-weight.h>
#include <fst/fst-decl.h>
#include <fst/lexicographic-weight.h>
#include <fst/power-weight.h>
#include <fst/string-weight.h>
#include <fst/tuple-weight.h>
#include <fst/weight.h>

// DEFINEs determine which semirings are tested; these are controlled by
// the `defines` attributes of the associated build rules.

DEFINE_uint64(seed, 403, "random seed");
DEFINE_int32(repeat, 25, "number of test repetitions");

namespace {

using fst::AlgoTester;
using fst::WeightGenerate;

}  // namespace

int main(int argc, char **argv) {
  SetFlag(&FST_FLAGS_fst_verify_properties, true);
  SET_FLAGS(argv[0], &argc, &argv, true);

  static const int kCacheGcLimit = 20;

  LOG(INFO) << "Seed = " << FST_FLAGS_seed;

  std::mt19937_64 rand(FST_FLAGS_seed);

  SetFlag(&FST_FLAGS_fst_default_cache_gc,
                std::bernoulli_distribution(.5)(rand));
  SetFlag(&FST_FLAGS_fst_default_cache_gc_limit,
                std::uniform_int_distribution<>(0, kCacheGcLimit)(rand));
  VLOG(1) << "default_cache_gc:" << FST_FLAGS_fst_default_cache_gc;
  VLOG(1) << "default_cache_gc_limit:"
          << FST_FLAGS_fst_default_cache_gc_limit;
#if defined(TEST_TROPICAL)
  using Arc = fst::StdArc;
#elif defined(TEST_LOG)
  using Arc = fst::LogArc;
#elif defined(TEST_MINMAX)
  using Arc = fst::MinMaxArc;
#elif defined(TEST_LEFT_STRING)
  using Arc = fst::StringArc<fst::STRING_LEFT>;
#elif defined(TEST_RIGHT_STRING)
  using Arc = fst::StringArc<fst::STRING_RIGHT>;
#elif defined(TEST_GALLIC)
  using Arc = fst::GallicArc<fst::StdArc>;
#elif defined(TEST_LEXICOGRAPHIC)
  using fst::LexicographicArc;
  using fst::TropicalWeight;
  using Arc = LexicographicArc<TropicalWeight, TropicalWeight>;
#elif defined(TEST_POWER)
  using fst::ArcTpl;
  using fst::PowerWeight;
  using fst::TropicalWeight;
  using Arc = ArcTpl<PowerWeight<TropicalWeight, 3>>;
#else
  #error "Must have one of the TEST_* macros defined."
#endif
  WeightGenerate<Arc::Weight> weight_generator(FST_FLAGS_seed,
                                               /*allow_zero=*/false);
  AlgoTester<Arc> arc_tester(weight_generator, FST_FLAGS_seed);
  arc_tester.Test();

  return 0;
}
