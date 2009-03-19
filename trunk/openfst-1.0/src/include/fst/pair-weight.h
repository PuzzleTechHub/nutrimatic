// pair-weight.h

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
// Author: shumash@google.com (Masha Maria Shugrina)
//
// \file
// Pair weight templated base class for weight classes that
// contain two weights (e.g. Product, Lexicographic)

#ifndef FST_LIB_PAIR_WEIGHT_H_
#define FST_LIB_PAIR_WEIGHT_H_

#include <stack>
#include <string>
#include <fst/weight.h>

DECLARE_string(fst_pair_parentheses);
DECLARE_string(fst_pair_separator);

namespace fst {

template<class W1, class W2> class PairWeight;
template <class W1, class W2>
istream &operator>>(istream &strm, PairWeight<W1, W2> &w);

template<class W1, class W2>
class PairWeight {
 public:
  typedef PairWeight<typename W1::ReverseWeight,
                     typename W2::ReverseWeight>
  ReverseWeight;

  PairWeight() {}

  PairWeight(const PairWeight& w) : value1_(w.value1_), value2_(w.value2_) {}

  PairWeight(W1 w1, W2 w2) : value1_(w1), value2_(w2) {}

  static const PairWeight<W1, W2> &Zero() {
    static const PairWeight<W1, W2> zero(W1::Zero(), W2::Zero());
    return zero;
  }

  static const PairWeight<W1, W2> &One() {
    static const PairWeight<W1, W2> one(W1::One(), W2::One());
    return one;
  }

  istream &Read(istream &strm) {
    value1_.Read(strm);
    return value2_.Read(strm);
  }

  ostream &Write(ostream &strm) const {
    value1_.Write(strm);
    return value2_.Write(strm);
  }

  PairWeight<W1, W2> &operator=(const PairWeight<W1, W2> &w) {
    value1_ = w.Value1();
    value2_ = w.Value2();
    return *this;
  }

  bool Member() const { return value1_.Member() && value2_.Member(); }

  size_t Hash() const {
    size_t h1 = value1_.Hash();
    size_t h2 = value2_.Hash();
    int lshift = 5;
    int rshift = sizeof(size_t) - 5;
    return h1 << lshift ^ h1 >> rshift ^ h2;
  }

  PairWeight<W1, W2> Quantize(float delta = kDelta) const {
    return PairWeight<W1, W2>(value1_.Quantize(delta),
                                 value2_.Quantize(delta));
  }

  ReverseWeight Reverse() const {
    return ReverseWeight(value1_.Reverse(), value2_.Reverse());
  }

  static uint64 Properties() {
    uint64 props1 = W1::Properties();
    uint64 props2 = W2::Properties();
    return props1 & props2 & (kLeftSemiring | kRightSemiring |
                              kCommutative | kIdempotent);
  }

  const W1& Value1() const { return value1_; }

  const W2& Value2() const { return value2_; }

 protected:
  // Reads PairWeight when there are not parentheses around pair terms
  inline static istream &ReadNoParen(
      istream &strm, PairWeight<W1, W2>& w, char separator) {
    int c;
    do {
      c = strm.get();
    } while (isspace(c));

    string s1;
    while (c != separator) {
      if (c == EOF) {
        strm.clear(std::ios::badbit);
        return strm;
      }
      s1 += c;
      c = strm.get();
    }
    istringstream strm1(s1);
    W1 w1 = W1::Zero();
    strm1 >> w1;

    // read second element
    W2 w2 = W2::Zero();
    strm >> w2;

    w = PairWeight<W1, W2>(w1, w2);
    return strm;
  }

  // Reads PairWeight when there are not parentheses around pair terms
  inline static istream &ReadWithParen(
      istream &strm, PairWeight<W1, W2>& w,
      char separator, char open_paren, char close_paren) {
    int c;
    do {
      c = strm.get();
    } while (isspace(c));
    if (c != open_paren)
      LOG(FATAL) << " is fst_pair_parentheses flag set correcty? ";
    c = strm.get();

    // read first element
    stack<int> parens;
    string s1;
    while (c != separator || !parens.empty()) {
      if (c == EOF) {
        strm.clear(std::ios::badbit);
        return strm;
      }
      s1 += c;
      // if parens encountered before separator, they must be matched
      if (c == open_paren) {
        parens.push(1);
      } else if (c == close_paren) {
        // Fail for mismatched parens
        if (parens.empty()) {
          strm.clear(std::ios::failbit);
          return strm;
        }
        parens.pop();
      }
      c = strm.get();
    }
    istringstream strm1(s1);
    W1 w1 = W1::Zero();
    strm1 >> w1;

    // read second element
    string s2;
    c = strm.get();
    while (c != EOF) {
      s2 += c;
      c = strm.get();
    }
    CHECK(s2.size() > 0);
    if (s2[s2.size() - 1] != close_paren)
      LOG(FATAL) << " is fst_pair_parentheses flag set correcty? ";
    s2.erase(s2.size() - 1, 1);
    istringstream strm2(s2);
    W2 w2 = W2::Zero();
    strm2 >> w2;

    w = PairWeight<W1, W2>(w1, w2);
    return strm;
  }

  W1 value1_;
  W2 value2_;
  friend istream &operator>><W1, W2>(istream&, PairWeight<W1, W2>&);
};

template <class W1, class W2>
inline bool operator==(const PairWeight<W1, W2> &w,
                       const PairWeight<W1, W2> &v) {
  return w.Value1() == v.Value1() && w.Value2() == v.Value2();
}

template <class W1, class W2>
inline bool operator!=(const PairWeight<W1, W2> &w1,
                       const PairWeight<W1, W2> &w2) {
  return w1.Value1() != w2.Value1() || w1.Value2() != w2.Value2();
}


template <class W1, class W2>
inline bool ApproxEqual(const PairWeight<W1, W2> &w1,
                        const PairWeight<W1, W2> &w2,
                        float delta = kDelta) {
  return ApproxEqual(w1.Value1(), w2.Value1(), kDelta) &&
      ApproxEqual(w1.Value2(), w2.Value2(), kDelta);
}

template <class W1, class W2>
inline ostream &operator<<(ostream &strm, const PairWeight<W1, W2> &w) {
  CHECK(FLAGS_fst_pair_separator.size() == 1);
  char separator = FLAGS_fst_pair_separator[0];
  if (FLAGS_fst_pair_parentheses.empty())
    return strm << w.Value1() << separator << w.Value2();

  CHECK(FLAGS_fst_pair_parentheses.size() == 2);
  char open_paren = FLAGS_fst_pair_parentheses[0];
  char close_paren = FLAGS_fst_pair_parentheses[1];
  return strm << open_paren << w.Value1() << separator
              << w.Value2() << close_paren ;
}

template <class W1, class W2>
inline istream &operator>>(istream &strm, PairWeight<W1, W2> &w) {
  CHECK(FLAGS_fst_pair_separator.size() == 1);
  char separator = FLAGS_fst_pair_separator[0];
  bool read_parens = !FLAGS_fst_pair_parentheses.empty();
  if (read_parens) {
    CHECK(FLAGS_fst_pair_parentheses.size() == 2);
    return PairWeight<W1, W2>::ReadWithParen(
        strm, w, separator, FLAGS_fst_pair_parentheses[0],
        FLAGS_fst_pair_parentheses[1]);
  } else {
    return PairWeight<W1, W2>::ReadNoParen(strm, w, separator);
  }
}

}

#endif  // FST_LIB_PAIR_WEIGHT_H_
