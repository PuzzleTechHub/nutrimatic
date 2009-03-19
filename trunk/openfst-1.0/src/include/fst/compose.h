// compose.h

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
// Class to compute the composition of two FSTs

#ifndef FST_LIB_COMPOSE_H__
#define FST_LIB_COMPOSE_H__

#include <algorithm>
#include <string>
#include <vector>

#include <fst/cache.h>
#include <fst/compose-filter.h>
#include <fst/map.h>
#include <fst/matcher.h>
#include <fst/state-table.h>
#include <fst/test-properties.h>

namespace fst {

// Delayed composition options templated on the arc type, the matcher,
// the composition filter, and the composition state table.
// By default, the matchers, filter, and state table are constructed
// by composition. If set below, the user can instead pass in these objects;
// in that case, ComposeFst takes their ownership.
template <class A,
          class M = Matcher<Fst<A> >,
          class F = SequenceComposeFilter<A>,
          class T = GenericComposeStateTable<A, typename F::FilterState> >
struct ComposeFstOptions : public CacheOptions {
  M *matcher1;      // FST1 matcher (see matcher.h)
  M *matcher2;      // FST2 matcher
  F *filter;        // Composition filter (see compose-filter.h)
  T *state_table;   // Composition state table (see compose-state-table.h)

  explicit ComposeFstOptions(const CacheOptions &opts,
                             M *mat1 = 0, M *mat2 = 0,
                             F *filt = 0, T *sttable= 0)
      : CacheOptions(opts), matcher1(mat1), matcher2(mat2),
        filter(filt), state_table(sttable) {}

  ComposeFstOptions() : matcher1(0), matcher2(0), filter(0), state_table(0) {}
};

// Implementation of delayed composition. This base class is
// common to the variants with different matchers, composition filters
// and state tables.
template <class A>
class ComposeFstImplBase : public CacheImpl<A> {
 public:
  using FstImpl<A>::SetType;
  using FstImpl<A>::SetProperties;
  using FstImpl<A>::Properties;
  using FstImpl<A>::SetInputSymbols;
  using FstImpl<A>::SetOutputSymbols;

  using CacheBaseImpl< CacheState<A> >::HasStart;
  using CacheBaseImpl< CacheState<A> >::HasFinal;
  using CacheBaseImpl< CacheState<A> >::HasArcs;

  typedef typename A::Label Label;
  typedef typename A::Weight Weight;
  typedef typename A::StateId StateId;
  typedef CacheState<A> State;

  ComposeFstImplBase(const Fst<A> &fst1,
                     const Fst<A> &fst2,
                     const CacheOptions &opts)
      :CacheImpl<A>(opts),
       fst1_(fst1.Copy()),
       fst2_(fst2.Copy()) {
    SetType("compose");

    if (!CompatSymbols(fst2.InputSymbols(), fst1.OutputSymbols()))
      LOG(FATAL) << "ComposeFst: output symbol table of 1st argument "
                 << "does not match input symbol table of 2nd argument";

    SetInputSymbols(fst1.InputSymbols());
    SetOutputSymbols(fst2.OutputSymbols());
  }

  ComposeFstImplBase(const ComposeFstImplBase<A> &impl)
      : CacheImpl<A>(impl),
        fst1_(impl.fst1_->Copy(true)),
        fst2_(impl.fst2_->Copy(true)) {
    SetProperties(impl.Properties(), kCopyProperties);
    SetInputSymbols(impl.InputSymbols());
    SetOutputSymbols(impl.OutputSymbols());
  }

  virtual ComposeFstImplBase<A> *Copy() = 0;

  virtual ~ComposeFstImplBase() {
    delete fst1_;
    delete fst2_;
  }

  StateId Start() {
    if (!HasStart()) {
      StateId start = ComputeStart();
      if (start != kNoStateId) {
        SetStart(start);
      }
    }
    return CacheImpl<A>::Start();
  }

  Weight Final(StateId s) {
    if (!HasFinal(s)) {
      Weight final = ComputeFinal(s);
      SetFinal(s, final);
    }
    return CacheImpl<A>::Final(s);
  }

  virtual void Expand(StateId s) = 0;

  size_t NumArcs(StateId s) {
    if (!HasArcs(s))
      Expand(s);
    return CacheImpl<A>::NumArcs(s);
  }

  size_t NumInputEpsilons(StateId s) {
    if (!HasArcs(s))
      Expand(s);
    return CacheImpl<A>::NumInputEpsilons(s);
  }

  size_t NumOutputEpsilons(StateId s) {
    if (!HasArcs(s))
      Expand(s);
    return CacheImpl<A>::NumOutputEpsilons(s);
  }

  void InitArcIterator(StateId s, ArcIteratorData<A> *data) {
    if (!HasArcs(s))
      Expand(s);
    CacheImpl<A>::InitArcIterator(s, data);
  }

 protected:
  virtual StateId ComputeStart() = 0;
  virtual Weight ComputeFinal(StateId s) = 0;

  const Fst<A> *fst1_;            // first input Fst
  const Fst<A> *fst2_;            // second input Fst
};


// Implementaion of delayed composition templated on the
// arc type, the matcher (see matcher.h),
// composition filter (see compose-filter.h) and the
// composition state table (see compose-state-table.h).
template <class A, class M, class F, class T>
class ComposeFstImpl : public ComposeFstImplBase<A> {
  typedef typename A::StateId StateId;
  typedef typename A::Label   Label;
  typedef typename A::Weight  Weight;
  typedef typename F::FilterState FilterState;
  using FstImpl<A>::SetType;
  using FstImpl<A>::SetProperties;
  using ComposeFstImplBase<A> ::fst1_;
  using ComposeFstImplBase<A> ::fst2_;

  typedef ComposeStateTuple<StateId, FilterState> StateTuple;


 public:
  ComposeFstImpl(const Fst<A> &fst1,
                 const Fst<A> &fst2,
                 const ComposeFstOptions<A, M, F, T> &opts)
      : ComposeFstImplBase<A>(fst1, fst2, opts),
        matcher1_(opts.matcher1 ? opts.matcher1 :  new M(fst1, MATCH_OUTPUT)),
        matcher2_(opts.matcher2 ? opts.matcher2 :  new M(fst2, MATCH_INPUT)),
        filter_(opts.filter ? opts.filter : new F(fst1, fst2)),
        state_table_(opts.state_table ? opts.state_table : new T(fst1, fst2)) {
    MatchType type1 = matcher1_->Type(false);
    MatchType type2 = matcher2_->Type(false);
    if (type1 == MATCH_OUTPUT && type2  == MATCH_INPUT) {
      match_type_ = MATCH_BOTH;
    } else if (type1 == MATCH_OUTPUT) {
      match_type_ = MATCH_OUTPUT;
    } else if (type2 == MATCH_INPUT) {
      match_type_ = MATCH_INPUT;
    } else if (matcher1_->Type(true) == MATCH_OUTPUT) {
      match_type_ = MATCH_OUTPUT;
    } else if (matcher2_->Type(true) == MATCH_INPUT) {
      match_type_ = MATCH_INPUT;
    } else {
      LOG(FATAL) << "ComposeFst: 1st argument cannot match on output labels "
                 << "and 2nd argument cannot match on input labels (sort?).";
    }
    uint64 fprops1 = fst1.Properties(kFstProperties, false);
    uint64 fprops2 = fst2.Properties(kFstProperties, false);
    uint64 mprops1 = matcher1_->Properties(fprops1);
    uint64 mprops2 = matcher2_->Properties(fprops2);
    SetProperties(ComposeProperties(mprops1, mprops2), kCopyProperties);
  }

  ComposeFstImpl(const ComposeFstImpl<A, M, F, T> &impl)
      : ComposeFstImplBase<A>(impl),
        matcher1_(new M(*impl.matcher1_)),
        matcher2_(new M(*impl.matcher2_)),
        filter_(new F(*impl.filter_)),
        state_table_(new T(*impl.state_table_)),
        match_type_(impl.match_type_) {}

  ~ComposeFstImpl() {
    delete matcher1_;
    delete matcher2_;
    delete filter_;
    delete state_table_;
  }

  virtual ComposeFstImpl<A, M, F, T> *Copy() {
    return new ComposeFstImpl<A, M, F, T>(*this);
  }

  // Arranges it so that the first arg to OrderedExpand is the Fst
  // that will be matched on.
  void Expand(StateId s) {
    const StateTuple &tuple = state_table_->Tuple(s);
    StateId s1 = tuple.state_id1;
    StateId s2 = tuple.state_id2;
    filter_->SetState(s1, s2, tuple.filter_state);
    if (match_type_ == MATCH_OUTPUT ||
        (match_type_ == MATCH_BOTH && fst1_->NumArcs(s1) > fst2_->NumArcs(s2)))
      OrderedExpand(s, fst1_, s1, fst2_, s2, matcher1_, false);
    else
      OrderedExpand(s, fst2_, s2, fst1_, s1, matcher2_, true);
  }

 private:
  // This does that actual matching of labels in the composition. The
  // arguments are ordered so matching is called on state 'sa' of
  // 'fsta' for each arc leaving state 'sb' of 'fstb'. The 'match_input' arg
  // determines whether the input or output label of arcs at 'sb' is
  // the one to match on.
  void OrderedExpand(StateId s, const Fst<A> *fsta, StateId sa,
                     const Fst<A> *fstb, StateId sb,
                     M *matchera,  bool match_input) {
    matchera->SetState(sa);

    // First process non-consuming symbols (e.g., epsilons) on FSTA.
    A loop(match_input ? 0 : kNoLabel, match_input ? kNoLabel : 0,
           Weight::One(), sb);
    MatchArc(s, matchera, loop, match_input);

    // Then process matches on FSTB.
    for (ArcIterator< Fst<A> > iterb(*fstb, sb); !iterb.Done(); iterb.Next())
      MatchArc(s, matchera, iterb.Value(), match_input);

    SetArcs(s);
  }

  // Matches a single transition from 'fstb' against 'fata' at 's'.
  void MatchArc(StateId s, M *matchera, const A &arc, bool match_input) {
    if (matchera->Find(match_input ? arc.olabel : arc.ilabel)) {
      for (; !matchera->Done(); matchera->Next()) {
        A arca = matchera->Value();
        A arcb = arc;
        if (match_input) {
          const FilterState &f = filter_->FilterArc(&arcb, &arca);
          if (f != FilterState::NoState())
            AddArc(s, arcb, arca, f);
        } else {
          const FilterState &f = filter_->FilterArc(&arca, &arcb);
          if (f != FilterState::NoState())
            AddArc(s, arca, arcb, f);
        }
      }
    }
  }

  // Add a matching transition at 's'.
  void AddArc(StateId s, const A &arc1, const A &arc2, const FilterState &f) {
    StateTuple tuple(arc1.nextstate, arc2.nextstate, f);
    A oarc(arc1.ilabel, arc2.olabel, Times(arc1.weight, arc2.weight),
           state_table_->FindState(tuple));
    CacheImpl<A>::AddArc(s, oarc);
  }

  StateId ComputeStart() {
    StateId s1 = fst1_->Start();
    StateId s2 = fst2_->Start();
    const FilterState &f = filter_->Start();
    if (s1 == kNoStateId || s2 == kNoStateId)
      return kNoStateId;
    StateTuple tuple(s1, s2, f);
    return state_table_->FindState(tuple);
  }

  Weight ComputeFinal(StateId s) {
    const StateTuple &tuple = state_table_->Tuple(s);
    StateId s1 = tuple.state_id1;
    StateId s2 = tuple.state_id2;
    filter_->SetState(s1, s2, tuple.filter_state);
    Weight final1 = fst1_->Final(s1);
    Weight final2 = fst2_->Final(s2);
    filter_->FilterFinal(&final1, &final2);
    return Times(final1, final2);
  }

  M *matcher1_;
  M *matcher2_;
  F *filter_;
  T *state_table_;

  MatchType match_type_;

  void operator=(const ComposeFstImpl<A, M, F, T> &);  // disallow
};

// Computes the composition of two transducers. This version is a
// delayed Fst. If FST1 transduces string x to y with weight a and FST2
// transduces y to z with weight b, then their composition transduces
// string x to z with weight Times(x, z).
//
// The output labels of the first transducer or the input labels of
// the second transducer must be sorted (with the default matcher).
// The weights need to form a commutative semiring (valid for
// TropicalWeight and LogWeight).
//
// Complexity:
// Assuming the first FST is unsorted and the second is sorted:
// - Time: O(v1 v2 d1 (log d2 + m2)),
// - Space: O(v1 v2)
// where vi = # of states visited, di = maximum out-degree, and mi the
// maximum multiplicity of the states visited for the ith
// FST. Constant time and space to visit an input state or arc is
// assumed and exclusive of caching.
//
// Caveats:
// - ComposeFst does not trim its output (since it is a delayed operation).
// - The efficiency of composition can be strongly affected by several factors:
//   - the choice of which tnansducer is sorted - prefer sorting the FST
//     that has the greater average out-degree.
//   - the amount of non-determinism
//   - the presence and location of epsilon transitions - avoid epsilon
//     transitions on the output side of the first transducer or
//     the input side of the second transducer or prefer placing
//     them later in a path since they delay matching and can
//     introduce non-coaccessible states and transitions.
template <class A>
class ComposeFst : public Fst<A> {
 public:
  friend class ArcIterator< ComposeFst<A> >;
  friend class CacheStateIterator< ComposeFst<A> >;
  friend class CacheArcIterator< ComposeFst<A> >;

  typedef A Arc;
  typedef typename A::Weight Weight;
  typedef typename A::StateId StateId;
  typedef CacheState<A> State;

  ComposeFst(const Fst<A> &fst1, const Fst<A> &fst2)
      : impl_(Init(fst1, fst2, ComposeFstOptions<A>())) {}

  template <class M, class F, class T>
  ComposeFst(const Fst<A> &fst1,
             const Fst<A> &fst2,
             const ComposeFstOptions<A, M, F, T> &opts)
      : impl_(Init(fst1, fst2, opts)) {}

  ComposeFst(const ComposeFst<A> &fst, bool reset = false) {
    if (reset) {
      impl_ = fst.impl_->Copy();
    } else {
      impl_ = fst.impl_;
      impl_->IncrRefCount();
    }
  }

  virtual ~ComposeFst() { if (!impl_->DecrRefCount()) delete impl_;  }

  virtual StateId Start() const { return impl_->Start(); }

  virtual Weight Final(StateId s) const { return impl_->Final(s); }

  virtual size_t NumArcs(StateId s) const { return impl_->NumArcs(s); }

  virtual size_t NumInputEpsilons(StateId s) const {
    return impl_->NumInputEpsilons(s);
  }

  virtual size_t NumOutputEpsilons(StateId s) const {
    return impl_->NumOutputEpsilons(s);
  }

  virtual uint64 Properties(uint64 mask, bool test) const {
    if (test) {
      uint64 known, test = TestProperties(*this, mask, &known);
      impl_->SetProperties(test, known);
      return test & mask;
    } else {
      return impl_->Properties(mask);
    }
  }

  virtual const string& Type() const { return impl_->Type(); }

  virtual ComposeFst<A> *Copy(bool reset = false) const {
    return new ComposeFst<A>(*this, reset);
  }

  virtual const SymbolTable* InputSymbols() const {
    return impl_->InputSymbols();
  }

  virtual const SymbolTable* OutputSymbols() const {
    return impl_->OutputSymbols();
  }

  virtual inline void InitStateIterator(StateIteratorData<A> *data) const;

  virtual void InitArcIterator(StateId s, ArcIteratorData<A> *data) const {
    impl_->InitArcIterator(s, data);
  }

 protected:
  ComposeFst() : impl_(0) {}

  ComposeFstImplBase<A> *Impl() { return impl_; }

  void SetImpl(ComposeFstImplBase<A> *impl) { impl_ = impl; }

  template <class M, class F, class T>
  static ComposeFstImplBase<A> *Init(
      const Fst<A> &fst1,
      const Fst<A> &fst2,
      const ComposeFstOptions<A, M, F, T> &opts) {

    if (!(Weight::Properties() & kCommutative)) {
      int64 props1 = fst1.Properties(kUnweighted, true);
      int64 props2 = fst2.Properties(kUnweighted, true);
      if (!(props1 & kUnweighted) && !(props2 & kUnweighted))
        LOG(FATAL) << "ComposeFst: Weight needs to be a commutative semiring: "
                   << Weight::Type();
    }

    return new ComposeFstImpl<A, M, F, T>(fst1, fst2, opts);
  }

 private:
  ComposeFstImplBase<A> *impl_;
  void operator=(const ComposeFst<A> &fst);  // disallow
};


// Specialization for ComposeFst.
template<class A>
class StateIterator< ComposeFst<A> >
    : public CacheStateIterator< ComposeFst<A> > {
 public:
  explicit StateIterator(const ComposeFst<A> &fst)
      : CacheStateIterator< ComposeFst<A> >(fst) {}
};


// Specialization for ComposeFst.
template <class A>
class ArcIterator< ComposeFst<A> >
    : public CacheArcIterator< ComposeFst<A> > {
 public:
  typedef typename A::StateId StateId;

  ArcIterator(const ComposeFst<A> &fst, StateId s)
      : CacheArcIterator< ComposeFst<A> >(fst, s) {
    if (!fst.impl_->HasArcs(s))
      fst.impl_->Expand(s);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ArcIterator);
};

template <class A> inline
void ComposeFst<A>::InitStateIterator(StateIteratorData<A> *data) const {
  data->base = new StateIterator< ComposeFst<A> >(*this);
}

// Useful alias when using StdArc.
typedef ComposeFst<StdArc> StdComposeFst;


struct ComposeOptions {
  bool connect;  // Connect output

  ComposeOptions(bool c) : connect(c) {}
  ComposeOptions() : connect(true) {}
};


// Computes the composition of two transducers. This version writes
// the composed FST into a MurableFst. If FST1 transduces string x to
// y with weight a and FST2 transduces y to z with weight b, then
// their composition transduces string x to z with weight
// Times(x, z).
//
// The output labels of the first transducer or the input labels of
// the second transducer must be sorted.  The weights need to form a
// commutative semiring (valid for TropicalWeight and LogWeight).
//
// Complexity:
// Assuming the first FST is unsorted and the second is sorted:
// - Time: O(V1 V2 D1 (log D2 + M2)),
// - Space: O(V1 V2 D1 M2)
// where Vi = # of states, Di = maximum out-degree, and Mi is
// the maximum multiplicity for the ith FST.
//
// Caveats:
// - Compose trims its output.
// - The efficiency of composition can be strongly affected by several factors:
//   - the choice of which tnansducer is sorted - prefer sorting the FST
//     that has the greater average out-degree.
//   - the amount of non-determinism
//   - the presence and location of epsilon transitions - avoid epsilon
//     transitions on the output side of the first transducer or
//     the input side of the second transducer or prefer placing
//     them later in a path since they delay matching and can
//     introduce non-coaccessible states and transitions.
template<class Arc>
void Compose(const Fst<Arc> &ifst1, const Fst<Arc> &ifst2,
             MutableFst<Arc> *ofst,
             const ComposeOptions &opts = ComposeOptions()) {
  ComposeFstOptions<Arc> nopts;
  nopts.gc_limit = 0;  // Cache only the last state for fastest copy.
  *ofst = ComposeFst<Arc>(ifst1, ifst2, nopts);
  if (opts.connect)
    Connect(ofst);
}

}  // namespace fst

#endif  // FST_LIB_COMPOSE_H__
