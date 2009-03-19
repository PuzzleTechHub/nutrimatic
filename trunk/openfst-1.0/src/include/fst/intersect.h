// intersect.h

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
// Class to compute the intersection of two FSAs

#ifndef FST_LIB_INTERSECT_H__
#define FST_LIB_INTERSECT_H__

#include <algorithm>
#include <vector>

#include <fst/cache.h>
#include <fst/compose.h>

namespace fst {

template <class A,
          class M = Matcher<Fst<A> >,
          class F = SequenceComposeFilter<A>,
          class T = GenericComposeStateTable<A, typename F::FilterState> >
struct IntersectFstOptions : public ComposeFstOptions<A, M, F, T> {
  explicit IntersectFstOptions(const CacheOptions &opts,
                               M *mat1 = 0, M *mat2 = 0,
                               F *filt = 0, T *sttable= 0)
      : ComposeFstOptions<A, M, F, T>(opts, mat1, mat2, filt, sttable) { }

  IntersectFstOptions() {}
};

// Computes the intersection (Hadamard product) of two FSAs. This
// version is a delayed Fst.  Only strings that are in both automata
// are retained in the result.
//
// The two arguments must be acceptors. One of the arguments must be
// label-sorted.
//
// Complexity: same as ComposeFst.
//
// Caveats:  same as ComposeFst.
template <class A>
class IntersectFst : public ComposeFst<A> {
 public:
  using ComposeFst<A>::Impl;

  typedef A Arc;
  typedef typename A::Weight Weight;
  typedef typename A::StateId StateId;

  IntersectFst(const Fst<A> &fst1, const Fst<A> &fst2)
      : ComposeFst<A>(fst1, fst2) {
    if (!fst1.Properties(kAcceptor, true) || !fst2.Properties(kAcceptor, true))
      LOG(FATAL) << "IntersectFst: arguments not both acceptors";
    uint64 props1 = fst1.Properties(kFstProperties, false);
    uint64 props2 = fst2.Properties(kFstProperties, false);
    Impl()->SetProperties(IntersectProperties(props1, props2),
                          kCopyProperties);
  }

  template <class M, class F, class T>
  IntersectFst(const Fst<A> &fst1, const Fst<A> &fst2,
               const IntersectFstOptions<A, M, F, T> &opts)
      : ComposeFst<A>(fst1, fst2, opts) {
    if (!fst1.Properties(kAcceptor, true) || !fst2.Properties(kAcceptor, true))
      LOG(FATAL) << "IntersectFst: arguments not both acceptors";
    uint64 props1 = fst1.Properties(kFstProperties, false);
    uint64 props2 = fst2.Properties(kFstProperties, false);
    Impl()->SetProperties(IntersectProperties(props1, props2),
                          kCopyProperties);
  }

  IntersectFst(const IntersectFst<A> &fst, bool reset = false) :
      ComposeFst<A>(fst, reset) {}

  virtual IntersectFst<A> *Copy(bool reset = false) const {
    return new IntersectFst<A>(*this, reset);
  }
};


// Specialization for IntersectFst.
template <class A>
class StateIterator< IntersectFst<A> >
    : public StateIterator< ComposeFst<A> > {
 public:
  explicit StateIterator(const IntersectFst<A> &fst)
      : StateIterator< ComposeFst<A> >(fst) {}
};


// Specialization for IntersectFst.
template <class A>
class ArcIterator< IntersectFst<A> >
    : public ArcIterator< ComposeFst<A> > {
 public:
  typedef typename A::StateId StateId;

  ArcIterator(const IntersectFst<A> &fst, StateId s)
      : ArcIterator< ComposeFst<A> >(fst, s) {}
};

// Useful alias when using StdArc.
typedef IntersectFst<StdArc> StdIntersectFst;


typedef ComposeOptions IntersectOptions;


// Computes the intersection (Hadamard product) of two FSAs. This
// version writes the intersection to an output MurableFst. Only
// strings that are in both automata are retained in the result.
//
// The two arguments must be acceptors. One of the arguments must be
// label-sorted.
//
// Complexity: same as Compose.
//
// Caveats:  same as Compose.
template<class Arc>
void Intersect(const Fst<Arc> &ifst1, const Fst<Arc> &ifst2,
             MutableFst<Arc> *ofst,
             const IntersectOptions &opts = IntersectOptions()) {
  IntersectFstOptions<Arc> nopts;
  nopts.gc_limit = 0;  // Cache only the last state for fastest copy.
  *ofst = IntersectFst<Arc>(ifst1, ifst2, nopts);
  if (opts.connect)
    Connect(ofst);
}

}  // namespace fst

#endif  // FST_LIB_INTERSECT_H__
