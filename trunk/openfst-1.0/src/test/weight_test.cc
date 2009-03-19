// weight_test.h

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Author: riley@google.com (Michael Riley)
//
// \file
// Regression test for Fst weights.

#include <cstdlib>
#include <ctime>

#include <fst/float-weight.h>
#include <fst/random-weight.h>
#include "./weight-tester.h"

DEFINE_int32(seed, -1, "random seed");
DEFINE_int32(repeat, 100000, "number of test repetitions");

using fst::TropicalWeight;
using fst::TropicalWeightGenerator;
using fst::TropicalWeightTpl;
using fst::TropicalWeightGenerator_;

using fst::LogWeight;
using fst::LogWeightGenerator;
using fst::LogWeightTpl;
using fst::LogWeightGenerator_;

using fst::MinMaxWeight;
using fst::MinMaxWeightGenerator;
using fst::MinMaxWeightTpl;
using fst::MinMaxWeightGenerator_;

using fst::StringWeight;
using fst::StringWeightGenerator;

using fst::GallicWeight;
using fst::GallicWeightGenerator;

using fst::LexicographicWeight;
using fst::LexicographicWeightGenerator;

using fst::ProductWeight;
using fst::ProductWeightGenerator;

using fst::STRING_LEFT;
using fst::STRING_RIGHT;

using fst::WeightTester;

template <class T>
void TestTemplatedWeights(int repeat, int seed) {
  TropicalWeightGenerator_<T> tropical_generator(seed);
  WeightTester<TropicalWeightTpl<T>, TropicalWeightGenerator_<T> >
      tropical_tester(tropical_generator);
  tropical_tester.Test(repeat);

  LogWeightGenerator_<T> log_generator(seed);
  WeightTester<LogWeightTpl<T>, LogWeightGenerator_<T> >
      log_tester(log_generator);
  log_tester.Test(repeat);

  MinMaxWeightGenerator_<T> minmax_generator(seed);
  WeightTester<MinMaxWeightTpl<T>, MinMaxWeightGenerator_<T> >
      minmax_tester(minmax_generator);
  minmax_tester.Test(repeat);
}

int main(int argc, char **argv) {
  std::set_new_handler(FailedNewHandler);
  SetFlags(argv[0], &argc, &argv, true);

  int seed = FLAGS_seed >= 0 ? FLAGS_seed : time(0);
  LOG(INFO) << "Seed = " << seed;

  // Test at full repetitions for float weights
  TestTemplatedWeights<float>(FLAGS_repeat, seed);
  // Test at reduced repetitions for double weights
  TestTemplatedWeights<double>(10, seed);
  // Make sure type names for templated weights are consistent
  CHECK(TropicalWeight::Type() == "tropical");
  CHECK(TropicalWeightTpl<double>::Type() != TropicalWeightTpl<float>::Type());
  CHECK(LogWeight::Type() == "log");
  CHECK(LogWeightTpl<double>::Type() != LogWeightTpl<float>::Type());
  TropicalWeightTpl<double> w(15.0);
  TropicalWeight tw(15.0);

  StringWeightGenerator<int> left_string_generator(seed);
  WeightTester<StringWeight<int>, StringWeightGenerator<int> >
    left_string_tester(left_string_generator);
  left_string_tester.Test(FLAGS_repeat);

  StringWeightGenerator<int, STRING_RIGHT> right_string_generator(seed);
  WeightTester<StringWeight<int, STRING_RIGHT>,
    StringWeightGenerator<int, STRING_RIGHT> >
    right_string_tester(right_string_generator);
  right_string_tester.Test(FLAGS_repeat);

  typedef GallicWeight<int, TropicalWeight> TropicalGallicWeight;
  typedef GallicWeightGenerator<int, TropicalWeightGenerator>
    TropicalGallicWeightGenerator;

  TropicalGallicWeightGenerator tropical_gallic_generator(seed);
  WeightTester<TropicalGallicWeight, TropicalGallicWeightGenerator>
    tropical_gallic_tester(tropical_gallic_generator);
  tropical_gallic_tester.Test(FLAGS_repeat);

  typedef ProductWeight<TropicalWeight, TropicalWeight> TropicalProductWeight;
  typedef ProductWeightGenerator<TropicalWeightGenerator,
      TropicalWeightGenerator> TropicalProductWeightGenerator;

  TropicalProductWeightGenerator tropical_product_generator(seed);
  WeightTester<TropicalProductWeight, TropicalProductWeightGenerator>
      tropical_product_weight_tester(tropical_product_generator);
  tropical_product_weight_tester.Test(FLAGS_repeat);

  typedef ProductWeight<TropicalWeight, TropicalProductWeight>
      SecondNestedProductWeight;
  typedef ProductWeightGenerator<TropicalWeightGenerator,
      TropicalProductWeightGenerator> SecondNestedProductWeightGenerator;

  SecondNestedProductWeightGenerator second_nested_product_generator(seed);
  WeightTester<SecondNestedProductWeight, SecondNestedProductWeightGenerator>
      second_nested_product_weight_tester(second_nested_product_generator);
  second_nested_product_weight_tester.Test(FLAGS_repeat);

  // This only works with fst_product_parentheses = "()"
  typedef ProductWeight<TropicalProductWeight, TropicalWeight>
      FirstNestedProductWeight;
  typedef ProductWeightGenerator<TropicalProductWeightGenerator,
      TropicalWeightGenerator> FirstNestedProductWeightGenerator;

  FirstNestedProductWeightGenerator first_nested_product_generator(seed);
  WeightTester<FirstNestedProductWeight, FirstNestedProductWeightGenerator>
      first_nested_product_weight_tester(first_nested_product_generator);

  // Test all product weight I/O with parentheses
  FLAGS_fst_pair_parentheses = "()";
  first_nested_product_weight_tester.Test(FLAGS_repeat);
  tropical_product_weight_tester.Test(5);
  second_nested_product_weight_tester.Test(5);
  tropical_gallic_tester.Test(5);
  FLAGS_fst_pair_parentheses = "";

  typedef LexicographicWeight<TropicalWeight, TropicalWeight>
      TropicalLexicographicWeight;
  typedef LexicographicWeightGenerator<TropicalWeightGenerator,
      TropicalWeightGenerator> TropicalLexicographicWeightGenerator;

  TropicalLexicographicWeightGenerator tropical_lexicographic_generator(seed);
  WeightTester<TropicalLexicographicWeight,
      TropicalLexicographicWeightGenerator>
    tropical_lexicographic_tester(tropical_lexicographic_generator);
  tropical_lexicographic_tester.Test(FLAGS_repeat);

  cout << "PASS" << endl;

  return 0;
}
