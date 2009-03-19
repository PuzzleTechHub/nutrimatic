// string-weight.h

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
// String weight set and associated semiring operation definitions.

#ifndef FST_LIB_STRING_WEIGHT_H__
#define FST_LIB_STRING_WEIGHT_H__

#include <list>
#include <string>

#include <fst/product-weight.h>
#include <fst/weight.h>

namespace fst {

const int kStringInfinity = -1;      // Label for the infinite string
const int kStringBad = -2;           // Label for a non-string
const char kStringSeparator = '_';   // Label separator in strings

// Determines whether to use left or right string semiring.  Includes
// restricted versions that signal an error if proper prefixes
// (suffixes) would otherwise be returned by Plus, useful with various
// algorithms that require functional transducer input with the
// string semirings.
enum StringType { STRING_LEFT = 0, STRING_RIGHT = 1 ,
                  STRING_LEFT_RESTRICT = 2, STRING_RIGHT_RESTRICT };

#define REVERSE_STRING_TYPE(S)                                  \
   ((S) == STRING_LEFT ? STRING_RIGHT :                         \
    ((S) == STRING_RIGHT ? STRING_LEFT :                        \
     ((S) == STRING_LEFT_RESTRICT ? STRING_RIGHT_RESTRICT :     \
      STRING_LEFT_RESTRICT)))

template <typename L, StringType S = STRING_LEFT>
class StringWeight;

template <typename L, StringType S = STRING_LEFT>
class StringWeightIterator;

template <typename L, StringType S = STRING_LEFT>
class StringWeightReverseIterator;

template <typename L, StringType S>
bool operator==(const StringWeight<L, S> &,  const StringWeight<L, S> &);


// String semiring: (longest_common_prefix/suffix, ., Infinity, Epsilon)
template <typename L, StringType S>
class StringWeight {
 public:
  typedef L Label;
  typedef StringWeight<L, REVERSE_STRING_TYPE(S)> ReverseWeight;

  friend class StringWeightIterator<L, S>;
  friend class StringWeightReverseIterator<L, S>;
  friend bool operator==<>(const StringWeight<L, S> &,
                           const StringWeight<L, S> &);

  StringWeight() { Init(); }

  template <typename Iter>
  StringWeight(const Iter &begin, const Iter &end) {
    Init();
    for (Iter iter = begin; iter != end; ++iter)
      PushBack(*iter);
  }

  explicit StringWeight(L l) { Init(); PushBack(l); }

  static const StringWeight<L, S> &Zero() {
    static const StringWeight<L, S> zero(kStringInfinity);
    return zero;
  }

  static const StringWeight<L, S> &One() {
    static const StringWeight<L, S> one;
    return one;
  }

  static const string &Type() {
    static const string type =
        S == STRING_LEFT ? "string" :
        (S == STRING_RIGHT ? "right_string" :
         (S == STRING_LEFT_RESTRICT ? "restricted_string" :
          "right_restricted_string"));
    return type;
  }

  bool Member() const;

  istream &Read(istream &strm);

  ostream &Write(ostream &strm) const;

  size_t Hash() const;

  StringWeight<L, S> Quantize(float delta = kDelta) const {
    return *this;
  }

  ReverseWeight Reverse() const;

  static uint64 Properties() {
    return (S == STRING_LEFT || S == STRING_LEFT_RESTRICT ?
            kLeftSemiring : kRightSemiring) | kIdempotent;
  }

  // NB: This needs to be uncommented only if default fails for this impl.
  // StringWeight<L, S> &operator=(const StringWeight<L, S> &w);

  // These operations combined with the StringWeightIterator and
  // StringWeightReverseIterator provide the access and mutation of
  // the string internal elements.

  // Common initializer among constructors.
  void Init() { first_ = 0; }

  // Clear existing StringWeight.
  void Clear() { first_ = 0; rest_.clear(); }

  size_t Size() const { return first_ ? rest_.size() + 1 : 0; }

  void PushFront(L l) {
    if (first_)
      rest_.push_front(first_);
    first_ = l;
  }

  void PushBack(L l) {
    if (!first_)
      first_ = l;
    else
      rest_.push_back(l);
  }

 private:
  L first_;         // first label in string (0 if empty)
  list<L> rest_;    // remaining labels in string
};


// Traverses string in forward direction.
template <typename L, StringType S>
class StringWeightIterator {
 public:
  explicit StringWeightIterator(const StringWeight<L, S>& w)
      : first_(w.first_), rest_(w.rest_), init_(true),
        iter_(rest_.begin()) {}

  bool Done() const {
    if (init_) return first_ == 0;
    else return iter_ == rest_.end();
  }

  const L& Value() const { return init_ ? first_ : *iter_; }

  void Next() {
    if (init_) init_ = false;
    else  ++iter_;
  }

  void Reset() {
    init_ = true;
    iter_ = rest_.begin();
  }

 private:
  const L &first_;
  const list<L> &rest_;
  bool init_;   // in the initialized state?
  typename list<L>::const_iterator iter_;

  DISALLOW_COPY_AND_ASSIGN(StringWeightIterator);
};


// Traverses string in forward direction.
template <typename L, StringType S>
class StringWeightReverseIterator {
 public:
  explicit StringWeightReverseIterator(const StringWeight<L, S>& w)
      : first_(w.first_), rest_(w.rest_), fin_(first_ == 0),
        iter_(rest_.rbegin()) {}

  bool Done() const { return fin_; }

  const L& Value() const { return iter_ == rest_.rend() ? first_ : *iter_; }

  void Next() {
    if (iter_ == rest_.rend()) fin_ = true;
    else  ++iter_;
  }

  void Reset() {
    fin_ = false;
    iter_ = rest_.rbegin();
  }

 private:
  const L &first_;
  const list<L> &rest_;
  bool fin_;   // in the final state?
  typename list<L>::const_reverse_iterator iter_;

  DISALLOW_COPY_AND_ASSIGN(StringWeightReverseIterator);
};


// StringWeight member functions follow that require
// StringWeightIterator or StringWeightReverseIterator.

template <typename L, StringType S>
inline istream &StringWeight<L, S>::Read(istream &strm) {
  Clear();
  int32 size;
  ReadType(strm, &size);
  for (int i = 0; i < size; ++i) {
    L label;
    ReadType(strm, &label);
    PushBack(label);
  }
  return strm;
}

template <typename L, StringType S>
inline ostream &StringWeight<L, S>::Write(ostream &strm) const {
  int32 size =  Size();
  WriteType(strm, size);
  for (StringWeightIterator<L, S> iter(*this); !iter.Done(); iter.Next()) {
    L label = iter.Value();
    WriteType(strm, label);
  }
  return strm;
}

template <typename L, StringType S>
inline bool StringWeight<L, S>::Member() const {
  if (Size() != 1)
    return true;
  StringWeightIterator<L, S> iter(*this);
  return iter.Value() != kStringBad;
}

template <typename L, StringType S>
inline typename StringWeight<L, S>::ReverseWeight
StringWeight<L, S>::Reverse() const {
  ReverseWeight rw;
  for (StringWeightIterator<L, S> iter(*this); !iter.Done(); iter.Next())
    rw.PushFront(iter.Value());
  return rw;
}

template <typename L, StringType S>
inline size_t StringWeight<L, S>::Hash() const {
  size_t h = 0;
  for (StringWeightIterator<L, S> iter(*this); !iter.Done(); iter.Next())
    h ^= h<<1 ^ iter.Value();
  return h;
}

// NB: This needs to be uncommented only if default fails for this the impl.
//
// template <typename L, StringType S>
// inline StringWeight<L, S>
// &StringWeight<L, S>::operator=(const StringWeight<L, S> &w) {
//   if (this != &w) {
//     Clear();
//     for (StringWeightIterator<L, S> iter(w); !iter.Done(); iter.Next())
//       PushBack(iter.Value());
//   }
//   return *this;
// }

template <typename L, StringType S>
inline bool operator==(const StringWeight<L, S> &w1,
                       const StringWeight<L, S> &w2) {
  if (w1.Size() != w2.Size())
    return false;

  StringWeightIterator<L, S> iter1(w1);
  StringWeightIterator<L, S> iter2(w2);

  for (; !iter1.Done() ; iter1.Next(), iter2.Next())
    if (iter1.Value() != iter2.Value())
      return false;

  return true;
}

template <typename L, StringType S>
inline bool operator!=(const StringWeight<L, S> &w1,
                       const StringWeight<L, S> &w2) {
  return !(w1 == w2);
}

template <typename L, StringType S>
inline bool ApproxEqual(const StringWeight<L, S> &w1,
                        const StringWeight<L, S> &w2,
                        float delta = kDelta) {
  return w1 == w2;
}

template <typename L, StringType S>
inline ostream &operator<<(ostream &strm, const StringWeight<L, S> &w) {
  StringWeightIterator<L, S> iter(w);
  if (iter.Done())
    return strm << "Epsilon";
  else if (iter.Value() == kStringInfinity)
    return strm << "Infinity";
  else if (iter.Value() == kStringBad)
    return strm << "BadString";
  else
    for (size_t i = 0; !iter.Done(); ++i, iter.Next()) {
      if (i > 0)
        strm << kStringSeparator;
      strm << iter.Value();
    }
  return strm;
}

template <typename L, StringType S>
inline istream &operator>>(istream &strm, StringWeight<L, S> &w) {
  string s;
  strm >> s;
  if (s == "Infinity") {
    w = StringWeight<L, S>::Zero();
  } else if (s == "Epsilon") {
    w = StringWeight<L, S>::One();
  } else {
    w.Clear();
    char *p = 0;
    for (const char *cs = s.c_str(); !p || *p != '\0'; cs = p + 1) {
      int l = strtoll(cs, &p, 10);
      if (p == cs || (*p != 0 && *p != kStringSeparator)) {
        strm.clear(std::ios::badbit);
        break;
      }
      w.PushBack(l);
    }
  }
  return strm;
}


// Default is for the restricted left and right semirings.  String
// equality is required (for non-Zero() input. This restriction
// is used in e.g. Determinize to ensure functional input.
template <typename L, StringType S>  inline StringWeight<L, S>
Plus(const StringWeight<L, S> &w1,
     const StringWeight<L, S> &w2) {
  if (w1 == StringWeight<L, S>::Zero())
    return w2;
  if (w2 == StringWeight<L, S>::Zero())
    return w1;

  if (w1 != w2)
    LOG(FATAL) << "StringWeight::Plus: unequal arguments "
               << "(non-functional FST?)";

  return w1;
}


// Longest common prefix for left string semiring.
template <typename L>  inline StringWeight<L, STRING_LEFT>
Plus(const StringWeight<L, STRING_LEFT> &w1,
     const StringWeight<L, STRING_LEFT> &w2) {
  if (w1 == StringWeight<L, STRING_LEFT>::Zero())
    return w2;
  if (w2 == StringWeight<L, STRING_LEFT>::Zero())
    return w1;

  StringWeight<L, STRING_LEFT> sum;
  StringWeightIterator<L, STRING_LEFT> iter1(w1);
  StringWeightIterator<L, STRING_LEFT> iter2(w2);
  for (; !iter1.Done() && !iter2.Done() && iter1.Value() == iter2.Value();
       iter1.Next(), iter2.Next())
    sum.PushBack(iter1.Value());
  return sum;
}


// Longest common suffix for right string semiring.
template <typename L>  inline StringWeight<L, STRING_RIGHT>
Plus(const StringWeight<L, STRING_RIGHT> &w1,
     const StringWeight<L, STRING_RIGHT> &w2) {
  if (w1 == StringWeight<L, STRING_RIGHT>::Zero())
    return w2;
  if (w2 == StringWeight<L, STRING_RIGHT>::Zero())
    return w1;

  StringWeight<L, STRING_RIGHT> sum;
  StringWeightReverseIterator<L, STRING_RIGHT> iter1(w1);
  StringWeightReverseIterator<L, STRING_RIGHT> iter2(w2);
  for (; !iter1.Done() && !iter2.Done() && iter1.Value() == iter2.Value();
       iter1.Next(), iter2.Next())
    sum.PushFront(iter1.Value());
  return sum;
}


template <typename L, StringType S>
inline StringWeight<L, S> Times(const StringWeight<L, S> &w1,
                             const StringWeight<L, S> &w2) {
  if (w1 == StringWeight<L, S>::Zero() || w2 == StringWeight<L, S>::Zero())
    return StringWeight<L, S>::Zero();

  StringWeight<L, S> prod(w1);
  for (StringWeightIterator<L, S> iter(w2); !iter.Done(); iter.Next())
    prod.PushBack(iter.Value());

  return prod;
}


// Default is for left division in the left string and the
// left restricted string semirings.
template <typename L, StringType S> inline StringWeight<L, S>
Divide(const StringWeight<L, S> &w1,
       const StringWeight<L, S> &w2,
       DivideType typ) {

  if (typ != DIVIDE_LEFT)
    LOG(FATAL) << "StringWeight::Divide: only left division is defined "
               << "for the " << StringWeight<L, S>::Type() << " semiring";

  if (w2 == StringWeight<L, S>::Zero())
    return StringWeight<L, S>(kStringBad);
  else if (w1 == StringWeight<L, S>::Zero())
    return StringWeight<L, S>::Zero();

  StringWeight<L, S> div;
  StringWeightIterator<L, S> iter(w1);
  for (int i = 0; !iter.Done(); iter.Next(), ++i) {
    if (i >= w2.Size())
      div.PushBack(iter.Value());
  }
  return div;
}


// Right division in the right string semiring.
template <typename L> inline StringWeight<L, STRING_RIGHT>
Divide(const StringWeight<L, STRING_RIGHT> &w1,
       const StringWeight<L, STRING_RIGHT> &w2,
       DivideType typ) {

  if (typ != DIVIDE_RIGHT)
    LOG(FATAL) << "StringWeight::Divide: only right division is defined "
               << "for the right string semiring";

  if (w2 == StringWeight<L, STRING_RIGHT>::Zero())
    return StringWeight<L, STRING_RIGHT>(kStringBad);
  else if (w1 == StringWeight<L, STRING_RIGHT>::Zero())
    return StringWeight<L, STRING_RIGHT>::Zero();

  StringWeight<L, STRING_RIGHT> div;
  StringWeightReverseIterator<L, STRING_RIGHT> iter(w1);
  for (int i = 0; !iter.Done(); iter.Next(), ++i) {
    if (i >= w2.Size())
      div.PushFront(iter.Value());
  }
  return div;
}


// Right division in the right restricted string semiring.
template <typename L> inline StringWeight<L, STRING_RIGHT_RESTRICT>
Divide(const StringWeight<L, STRING_RIGHT_RESTRICT> &w1,
       const StringWeight<L, STRING_RIGHT_RESTRICT> &w2,
       DivideType typ) {

  if (typ != DIVIDE_RIGHT)
    LOG(FATAL) << "StringWeight::Divide: only right division is defined "
               << "for the right restricted string semiring";

  if (w2 == StringWeight<L, STRING_RIGHT_RESTRICT>::Zero())
    return StringWeight<L, STRING_RIGHT_RESTRICT>(kStringBad);
  else if (w1 == StringWeight<L, STRING_RIGHT_RESTRICT>::Zero())
    return StringWeight<L, STRING_RIGHT_RESTRICT>::Zero();

  StringWeight<L, STRING_RIGHT_RESTRICT> div;
  StringWeightReverseIterator<L, STRING_RIGHT_RESTRICT> iter(w1);
  for (int i = 0; !iter.Done(); iter.Next(), ++i) {
    if (i >= w2.Size())
      div.PushFront(iter.Value());
  }
  return div;
}


// Product of string weight and an arbitray weight.
template <class L, class W, StringType S = STRING_LEFT>
struct GallicWeight : public ProductWeight<StringWeight<L, S>, W> {
  typedef GallicWeight<L, typename W::ReverseWeight, REVERSE_STRING_TYPE(S)>
  ReverseWeight;

  GallicWeight() {}

  GallicWeight(StringWeight<L, S> w1, W w2)
      : ProductWeight<StringWeight<L, S>, W>(w1, w2) {}

  explicit GallicWeight(const string &s, int *nread = 0)
      : ProductWeight<StringWeight<L, S>, W>(s, nread) {}

  GallicWeight(const ProductWeight<StringWeight<L, S>, W> &w)
      : ProductWeight<StringWeight<L, S>, W>(w) {}
};

}  // namespace fst;

#endif  // FST_LIB_STRING_WEIGHT_H__
