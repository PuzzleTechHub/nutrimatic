// random-weight.h

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
// Function objects to generate random weights in various semirings
// for testing purposes.

#ifndef FST_LIB_RANDOM_WEIGHT_H__
#define FST_LIB_RANDOM_WEIGHT_H__

#include <cstdlib>
#include <ctime>
#include <vector>

#include <fst/float-weight.h>
#include <fst/product-weight.h>
#include <fst/string-weight.h>
#include <fst/lexicographic-weight.h>

namespace fst {

// The boolean 'allow_zero' below determines whether Zero() and zero
// divisors should be returned in the random weight generation.

// This function object returns TropicalWeightTpl<T>'s that are random integers
// chosen from [0, kNumRandomWeights).
template <class T>
class TropicalWeightGenerator_ {
 public:
  typedef TropicalWeightTpl<T> Weight;

  TropicalWeightGenerator_(int seed = time(0), bool allow_zero = true)
      : allow_zero_(allow_zero) {
    srand(seed);
  }

  Weight operator() () const {
    int n = rand() % (kNumRandomWeights + allow_zero_);
    if (allow_zero_ && n == kNumRandomWeights)
      return Weight::Zero();

    return Weight(static_cast<T>(n));
  }

 private:
  // The number of alternative random weights.
  static const int kNumRandomWeights = 5;

  bool allow_zero_;  // permit Zero() and zero divisors
};

typedef TropicalWeightGenerator_<float> TropicalWeightGenerator;


// This function object returns LogWeightTpl<T>'s that are random integers
// chosen from [0, kNumRandomWeights).
template <class T>
class LogWeightGenerator_ {
 public:
  typedef LogWeightTpl<T> Weight;

  LogWeightGenerator_(int seed = time(0), bool allow_zero = true)
      : allow_zero_(allow_zero) {
    srand(seed);
  }

  Weight operator() () const {
    int n = rand() % (kNumRandomWeights + allow_zero_);
    if (allow_zero_ && n == kNumRandomWeights)
      return Weight::Zero();

    return Weight(static_cast<T>(n));
  }

 private:
  // Number of alternative random weights.
  static const int kNumRandomWeights = 5;

  bool allow_zero_;  // permit Zero() and zero divisors
};

typedef LogWeightGenerator_<float> LogWeightGenerator;


// This function object returns MinMaxWeightTpl<T>'s that are random integers
// chosen from (-kNumRandomWeights, kNumRandomWeights) in addition to
// One(), and Zero() if zero is allowed.
template <class T>
class MinMaxWeightGenerator_ {
 public:
  typedef MinMaxWeightTpl<T> Weight;

  MinMaxWeightGenerator_(int seed = time(0), bool allow_zero = true)
      : allow_zero_(allow_zero) {
    srand(seed);
  }

  Weight operator() () const {
    int n = (rand() % (2*kNumRandomWeights + allow_zero_)) - kNumRandomWeights;
    if (allow_zero_ && n == kNumRandomWeights)
      return Weight::Zero();
    else if (n == -kNumRandomWeights)
      return Weight::One();

    return Weight(static_cast<T>(n));
  }

 private:
  // Parameters controlling the number of alternative random weights.
  static const int kNumRandomWeights = 5;

  bool allow_zero_;  // permit Zero() and zero divisors
};

typedef MinMaxWeightGenerator_<float> MinMaxWeightGenerator;


// This function object returns StringWeights that are random integer
// strings chosen from {1,...,kAlphabetSize}^{0,kMaxStringLength} U { Zero }
template <typename L, StringType S = STRING_LEFT>
class StringWeightGenerator {
 public:
  typedef StringWeight<L, S> Weight;

  StringWeightGenerator(int seed = time(0), bool allow_zero = true)
      : allow_zero_(allow_zero) {
     srand(seed);
  }

  Weight operator() () const {
    int n = rand() % (kMaxStringLength + allow_zero_);
    if (allow_zero_ && n == kMaxStringLength)
      return Weight::Zero();

    vector<L> v;
    for (int i = 0; i < n; ++i)
      v.push_back(rand() % kAlphabetSize + 1);
    return Weight(v.begin(), v.end());
  }

 private:
  // Alphabet size for random weights.
  static const int kAlphabetSize = 5;
  // Number of alternative random weights.
  static const int kMaxStringLength = 5;

  bool allow_zero_;  // permit Zero() and zero divisors
};


// This function object returns a weight generator over the product of the
// weights for the generators G1 and G2.
template <class G1, class G2>
class ProductWeightGenerator {
 public:
  typedef typename G1::Weight W1;
  typedef typename G2::Weight W2;
  typedef ProductWeight<W1, W2> Weight;

  ProductWeightGenerator(int seed = time(0), bool allow_zero = true)
      : generator1_(seed, allow_zero), generator2_(seed, allow_zero) {}

  Weight operator() () const {
    W1 w1 = generator1_();
    W2 w2 = generator2_();
    return Weight(w1, w2);
  }

 private:
  G1 generator1_;
  G2 generator2_;
};


// This function object returns a weight generator for a lexicographic weight
// composed out of weights for the generators G1 and G2. For lexicographic
// weights, we cannot generate zeroes for the two subweights separately:
// weights are members iff both members are zero or both members are non-zero.
template <class G1, class G2>
class LexicographicWeightGenerator {
 public:
  typedef typename G1::Weight W1;
  typedef typename G2::Weight W2;
  typedef LexicographicWeight<W1, W2> Weight;

  LexicographicWeightGenerator(int seed = time(0), bool allow_zero = true)
      : generator1_(seed, false), generator2_(seed, false),
        allow_zero_(allow_zero) {}

  Weight operator() () const {
    if (allow_zero_) {
      int n = rand() % (kNumRandomWeights + allow_zero_);
      if (n == kNumRandomWeights)
        return Weight(W1::Zero(), W2::Zero());
    }
    W1 w1 = generator1_();
    W2 w2 = generator2_();
    return Weight(w1, w2);
  }

 private:
  G1 generator1_;
  G2 generator2_;
  static const int kNumRandomWeights = 5;
  bool allow_zero_;
};

// Product generator of a string weight generator and an
// arbitrary weight generator.
template <class L, class G, StringType S = STRING_LEFT>
class GallicWeightGenerator
    : public ProductWeightGenerator<StringWeightGenerator<L, S>, G> {

 public:
  typedef ProductWeightGenerator<StringWeightGenerator<L, S>, G> PG;
  typedef typename G::Weight W;
  typedef GallicWeight<L, W, S> Weight;

  GallicWeightGenerator(int seed = time(0), bool allow_zero = true)
      : PG(seed, allow_zero) {}

  GallicWeightGenerator(const PG &pg) : PG(pg) {}
};

}  // namespace fst;

#endif  // FST_LIB_RANDOM_WEIGHT_H__
