// union.h

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
// Functions and classes to compute the union of two FSTs.

#ifndef FST_LIB_UNION_H__
#define FST_LIB_UNION_H__

#include <vector>
#include <algorithm>
#include <fst/mutable-fst.h>
#include <fst/rational.h>

namespace fst {

// Computes the union (sum) of two FSTs.  This version writes the
// union to an output MurableFst. If A transduces string x to y with
// weight a and B transduces string w to v with weight b, then their
// union transduces x to y with weight a and w to v with weight b.
//
// Complexity:
// - Time: (V2 + E2)
// - Space: O(V2 + E2)
// where Vi = # of states and Ei = # of arcs of the ith FST.
template <class Arc>
void Union(MutableFst<Arc> *fst1, const Fst<Arc> &fst2) {
  typedef typename Arc::StateId StateId;
  typedef typename Arc::Label Label;
  typedef typename Arc::Weight Weight;

  StateId start2 = fst2.Start();
  if (start2 == kNoStateId)
    return;

  StateId numstates1 = fst1->NumStates();
  bool initial_acyclic1 = fst1->Properties(kInitialAcyclic, true);
  uint64 props1 = fst1->Properties(kFstProperties, false);
  uint64 props2 = fst2.Properties(kFstProperties, false);

  for (StateIterator< Fst<Arc> > siter(fst2);
       !siter.Done();
       siter.Next()) {
    StateId s1 = fst1->AddState();
    StateId s2 = siter.Value();
    fst1->SetFinal(s1, fst2.Final(s2));
    for (ArcIterator< Fst<Arc> > aiter(fst2, s2);
         !aiter.Done();
         aiter.Next()) {
      Arc arc = aiter.Value();
      arc.nextstate += numstates1;
      fst1->AddArc(s1, arc);
    }
  }
  StateId start1 = fst1->Start();
  if (start1 == kNoStateId) {
    fst1->SetStart(start2);
    fst1->SetProperties(props2, kCopyProperties);
    return;
  }

  if (initial_acyclic1) {
    fst1->AddArc(start1,  Arc(0, 0, Weight::One(), start2 + numstates1));
  } else {
    StateId nstart1 = fst1->AddState();
    fst1->SetStart(nstart1);
    fst1->AddArc(nstart1,  Arc(0, 0, Weight::One(), start1));
    fst1->AddArc(nstart1,  Arc(0, 0, Weight::One(), start2 + numstates1));
  }
  fst1->SetProperties(UnionProperties(props1, props2), kFstProperties);
}


// Computes the union of two FSTs; this version modifies its
// RationalFst argument.
template<class Arc>
void Union(RationalFst<Arc> *fst1, const Fst<Arc> &fst2) {
  fst1->Impl()->AddUnion(fst2);
}


typedef RationalFstOptions UnionFstOptions;


// Computes the union (sum) of two FSTs. This version is a delayed
// Fst. If A transduces string x to y with weight a and B transduces
// string w to v with weight b, then their union transduces x to y
// with weight a and w to v with weight b.
//
// Complexity:
// - Time: O(v1 + e1 + v2 + e2)
// - Sapce: O(v1 + v2)
// where vi = # of states visited and ei = # of arcs visited of the
// ith FST. Constant time and space to visit an input state or arc
// is assumed and exclusive of caching.
template <class A>
class UnionFst : public RationalFst<A> {
 public:
  using RationalFst<A>::Impl;

  typedef A Arc;
  typedef typename A::Weight Weight;
  typedef typename A::StateId StateId;

  UnionFst(const Fst<A> &fst1, const Fst<A> &fst2) {
    Impl()->InitUnion(fst1, fst2);
  }

  UnionFst(const Fst<A> &fst1, const Fst<A> &fst2, const UnionFstOptions &opts)
      : RationalFst<A>(opts) {
    Impl()->InitUnion(fst1, fst2);
  }

  UnionFst(const UnionFst<A> &fst, bool reset = false)
      : RationalFst<A>(fst, reset) {}

  virtual UnionFst<A> *Copy(bool reset = false) const {
    return new UnionFst<A>(*this, reset);
  }
};


// Specialization for UnionFst.
template <class A>
class StateIterator< UnionFst<A> > : public StateIterator< RationalFst<A> > {
 public:
  explicit StateIterator(const UnionFst<A> &fst)
      : StateIterator< RationalFst<A> >(fst) {}
};


// Specialization for UnionFst.
template <class A>
class ArcIterator< UnionFst<A> > : public ArcIterator< RationalFst<A> > {
 public:
  typedef typename A::StateId StateId;

  ArcIterator(const UnionFst<A> &fst, StateId s)
      : ArcIterator< RationalFst<A> >(fst, s) {}
};


// Useful alias when using StdArc.
typedef UnionFst<StdArc> StdUnionFst;

}  // namespace fst

#endif  // FST_LIB_UNION_H__
