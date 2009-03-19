// factor-weight.h

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
// Author: allauzen@google.com (Cyril Allauzen)
//
// \file
// Classes to factor weights in an FST.

#ifndef FST_LIB_FACTOR_WEIGHT_H__
#define FST_LIB_FACTOR_WEIGHT_H__

#include <algorithm>
#include <tr1/unordered_map>
using std::tr1::unordered_map;
#include <fst/slist.h>
#include <string>
#include <utility>
#include <vector>

#include <fst/cache.h>
#include <fst/test-properties.h>

namespace fst {

const uint32 kFactorFinalWeights = 0x00000001;
const uint32 kFactorArcWeights   = 0x00000002;

template <class Arc>
struct FactorWeightOptions : CacheOptions {
  typedef typename Arc::Label Label;
  float delta;
  uint32 mode;         // factor arc weights and/or final weights
  Label final_ilabel;  // input label of arc created when factoring final w's
  Label final_olabel;  // output label of arc created when factoring final w's

  FactorWeightOptions(const CacheOptions &opts, float d,
                      uint32 m = kFactorArcWeights | kFactorFinalWeights,
                      Label il = 0, Label ol = 0)
      : CacheOptions(opts), delta(d), mode(m), final_ilabel(il),
        final_olabel(ol) {}

  explicit FactorWeightOptions(
      float d, uint32 m = kFactorArcWeights | kFactorFinalWeights,
      Label il = 0, Label ol = 0)
      : delta(d), mode(m), final_ilabel(il), final_olabel(ol) {}

  FactorWeightOptions(uint32 m = kFactorArcWeights | kFactorFinalWeights,
                      Label il = 0, Label ol = 0)
      : delta(kDelta), mode(m), final_ilabel(il), final_olabel(ol) {}
};


// A factor iterator takes as argument a weight w and returns a
// sequence of pairs of weights (xi,yi) such that the sum of the
// products xi times yi is equal to w. If w is fully factored,
// the iterator should return nothing.
//
// template <class W>
// class FactorIterator {
//  public:
//   FactorIterator(W w);
//   bool Done() const;
//   void Next();
//   pair<W, W> Value() const;
//   void Reset();
// }


// Factor trivially.
template <class W>
class IdentityFactor {
 public:
  IdentityFactor(const W &w) {}
  bool Done() const { return true; }
  void Next() {}
  pair<W, W> Value() const { return make_pair(W::One(), W::One()); } // unused
  void Reset() {}
};


// Factor a StringWeight w as 'ab' where 'a' is a label.
template <typename L, StringType S = STRING_LEFT>
class StringFactor {
 public:
  StringFactor(const StringWeight<L, S> &w)
      : weight_(w), done_(w.Size() <= 1) {}

  bool Done() const { return done_; }

  void Next() { done_ = true; }

  pair< StringWeight<L, S>, StringWeight<L, S> > Value() const {
    StringWeightIterator<L, S> iter(weight_);
    StringWeight<L, S> w1(iter.Value());
    StringWeight<L, S> w2;
    for (iter.Next(); !iter.Done(); iter.Next())
      w2.PushBack(iter.Value());
    return make_pair(w1, w2);
  }

  void Reset() { done_ = weight_.Size() <= 1; }

 private:
  StringWeight<L, S> weight_;
  bool done_;
};


// Factor a GallicWeight using StringFactor.
template <class L, class W, StringType S = STRING_LEFT>
class GallicFactor {
 public:
  GallicFactor(const GallicWeight<L, W, S> &w)
      : weight_(w), done_(w.Value1().Size() <= 1) {}

  bool Done() const { return done_; }

  void Next() { done_ = true; }

  pair< GallicWeight<L, W, S>, GallicWeight<L, W, S> > Value() const {
    StringFactor<L, S> iter(weight_.Value1());
    GallicWeight<L, W, S> w1(iter.Value().first, weight_.Value2());
    GallicWeight<L, W, S> w2(iter.Value().second, W::One());
    return make_pair(w1, w2);
  }

  void Reset() { done_ = weight_.Value1().Size() <= 1; }

 private:
  GallicWeight<L, W, S> weight_;
  bool done_;
};


// Implementation class for FactorWeight
template <class A, class F>
class FactorWeightFstImpl
    : public CacheImpl<A> {
 public:
  using FstImpl<A>::SetType;
  using FstImpl<A>::SetProperties;
  using FstImpl<A>::Properties;
  using FstImpl<A>::SetInputSymbols;
  using FstImpl<A>::SetOutputSymbols;

  using CacheBaseImpl< CacheState<A> >::HasStart;
  using CacheBaseImpl< CacheState<A> >::HasFinal;
  using CacheBaseImpl< CacheState<A> >::HasArcs;

  typedef A Arc;
  typedef typename A::Label Label;
  typedef typename A::Weight Weight;
  typedef typename A::StateId StateId;
  typedef F FactorIterator;

  struct Element {
    Element() {}

    Element(StateId s, Weight w) : state(s), weight(w) {}

    StateId state;     // Input state Id
    Weight weight;     // Residual weight
  };

  FactorWeightFstImpl(const Fst<A> &fst, const FactorWeightOptions<A> &opts)
      : CacheImpl<A>(opts),
        fst_(fst.Copy()),
        delta_(opts.delta),
        mode_(opts.mode),
        final_ilabel_(opts.final_ilabel),
        final_olabel_(opts.final_olabel) {
    SetType("factor-weight");
    uint64 props = fst.Properties(kFstProperties, false);
    SetProperties(FactorWeightProperties(props), kCopyProperties);

    SetInputSymbols(fst.InputSymbols());
    SetOutputSymbols(fst.OutputSymbols());

    if (mode_ == 0)
      LOG(WARNING) << "FactorWeightFst: factor mode is set to 0: "
                   << "factoring neither arc weights nor final weights.";
  }

  FactorWeightFstImpl(const FactorWeightFstImpl<A, F> &impl)
      : CacheImpl<A>(impl),
        fst_(impl.fst_->Copy(true)),
        delta_(impl.delta_),
        mode_(impl.mode_),
        final_ilabel_(impl.final_ilabel_),
        final_olabel_(impl.final_olabel_) {
    SetType("factor-weight");
    SetProperties(impl.Properties(), kCopyProperties);
    SetInputSymbols(impl.InputSymbols());
    SetOutputSymbols(impl.OutputSymbols());
  }

  ~FactorWeightFstImpl() {
    delete fst_;
  }

  StateId Start() {
    if (!HasStart()) {
      StateId s = fst_->Start();
      if (s == kNoStateId)
        return kNoStateId;
      StateId start = FindState(Element(fst_->Start(), Weight::One()));
      SetStart(start);
    }
    return CacheImpl<A>::Start();
  }

  Weight Final(StateId s) {
    if (!HasFinal(s)) {
      const Element &e = elements_[s];
      // TODO: fix so cast is unnecessary
      Weight w = e.state == kNoStateId
                 ? e.weight
                 : (Weight) Times(e.weight, fst_->Final(e.state));
      FactorIterator f(w);
      if (!(mode_ & kFactorFinalWeights) || f.Done())
        SetFinal(s, w);
      else
        SetFinal(s, Weight::Zero());
    }
    return CacheImpl<A>::Final(s);
  }

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


  // Find state corresponding to an element. Create new state
  // if element not found.
  StateId FindState(const Element &e) {
    if (!(mode_ & kFactorArcWeights) && e.weight == Weight::One()) {
      while (unfactored_.size() <= e.state)
        unfactored_.push_back(kNoStateId);
      if (unfactored_[e.state] == kNoStateId) {
        unfactored_[e.state] = elements_.size();
        elements_.push_back(e);
      }
      return unfactored_[e.state];
    } else {
      typename ElementMap::iterator eit = element_map_.find(e);
      if (eit != element_map_.end()) {
        return (*eit).second;
      } else {
        StateId s = elements_.size();
        elements_.push_back(e);
        element_map_.insert(pair<const Element, StateId>(e, s));
        return s;
      }
    }
  }

  // Computes the outgoing transitions from a state, creating new destination
  // states as needed.
  void Expand(StateId s) {
    Element e = elements_[s];
    if (e.state != kNoStateId) {
      for (ArcIterator< Fst<A> > ait(*fst_, e.state);
           !ait.Done();
           ait.Next()) {
        const A &arc = ait.Value();
        Weight w = Times(e.weight, arc.weight);
        FactorIterator fit(w);
        if (!(mode_ & kFactorArcWeights) || fit.Done()) {
          StateId d = FindState(Element(arc.nextstate, Weight::One()));
          AddArc(s, Arc(arc.ilabel, arc.olabel, w, d));
        } else {
          for (; !fit.Done(); fit.Next()) {
            const pair<Weight, Weight> &p = fit.Value();
            StateId d = FindState(Element(arc.nextstate,
                                          p.second.Quantize(delta_)));
            AddArc(s, Arc(arc.ilabel, arc.olabel, p.first, d));
          }
        }
      }
    }

    if ((mode_ & kFactorFinalWeights) &&
        ((e.state == kNoStateId) ||
         (fst_->Final(e.state) != Weight::Zero()))) {
      Weight w = e.state == kNoStateId
                 ? e.weight
                 : Times(e.weight, fst_->Final(e.state));
      for (FactorIterator fit(w);
           !fit.Done();
           fit.Next()) {
        const pair<Weight, Weight> &p = fit.Value();
        StateId d = FindState(Element(kNoStateId,
                                      p.second.Quantize(delta_)));
        AddArc(s, Arc(final_ilabel_, final_olabel_, p.first, d));
      }
    }
    SetArcs(s);
  }

 private:
  // Equality function for Elements, assume weights have been quantized.
  class ElementEqual {
   public:
    bool operator()(const Element &x, const Element &y) const {
      return x.state == y.state && x.weight == y.weight;
    }
  };

  // Hash function for Elements to Fst states.
  class ElementKey {
   public:
    size_t operator()(const Element &x) const {
      return static_cast<size_t>(x.state * kPrime + x.weight.Hash());
    }
   private:
    static const int kPrime = 7853;
  };

  typedef unordered_map<Element, StateId, ElementKey, ElementEqual> ElementMap;

  const Fst<A> *fst_;
  float delta_;
  uint32 mode_;               // factoring arc and/or final weights
  Label final_ilabel_;        // ilabel of arc created when factoring final w's
  Label final_olabel_;        // olabel of arc created when factoring final w's
  vector<Element> elements_;  // mapping Fst state to Elements
  ElementMap element_map_;    // mapping Elements to Fst state
  // mapping between old/new 'StateId' for states that do not need to
  // be factored when 'mode_' is '0' or 'kFactorFinalWeights'
  vector<StateId> unfactored_;

  void operator=(const FactorWeightFstImpl<A, F> &);  // disallow
};


// FactorWeightFst takes as template parameter a FactorIterator as
// defined above. The result of weight factoring is a transducer
// equivalent to the input whose path weights have been factored
// according to the FactorIterator. States and transitions will be
// added as necessary. The algorithm is a generalization to arbitrary
// weights of the second step of the input epsilon-normalization
// algorithm due to Mohri, "Generic epsilon-removal and input
// epsilon-normalization algorithms for weighted transducers",
// International Journal of Computer Science 13(1): 129-143 (2002).
template <class A, class F>
class FactorWeightFst : public Fst<A> {
 public:
  friend class ArcIterator< FactorWeightFst<A, F> >;
  friend class CacheStateIterator< FactorWeightFst<A, F> >;
  friend class CacheArcIterator< FactorWeightFst<A, F> >;

  typedef A Arc;
  typedef typename A::Weight Weight;
  typedef typename A::StateId StateId;
  typedef CacheState<A> State;

  FactorWeightFst(const Fst<A> &fst)
      : impl_(new FactorWeightFstImpl<A, F>(fst, FactorWeightOptions<A>())) {}

  FactorWeightFst(const Fst<A> &fst,  const FactorWeightOptions<A> &opts)
      : impl_(new FactorWeightFstImpl<A, F>(fst, opts)) {}

  FactorWeightFst(const FactorWeightFst<A, F> &fst, bool reset) {
    if (reset) {
      impl_ = new FactorWeightFstImpl<A, F>(*(fst.impl_));
    } else {
      impl_ = fst.impl_;
      impl_->IncrRefCount();
    }
  }

  virtual ~FactorWeightFst() { if (!impl_->DecrRefCount()) delete impl_;  }

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

  virtual FactorWeightFst<A, F> *Copy(bool reset = false) const {
    return new FactorWeightFst<A, F>(*this, reset);
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

 private:
  FactorWeightFstImpl<A, F> *Impl() { return impl_; }

  FactorWeightFstImpl<A, F> *impl_;

  void operator=(const FactorWeightFst<A, F> &fst);  // Disallow
};


// Specialization for FactorWeightFst.
template<class A, class F>
class StateIterator< FactorWeightFst<A, F> >
    : public CacheStateIterator< FactorWeightFst<A, F> > {
 public:
  explicit StateIterator(const FactorWeightFst<A, F> &fst)
      : CacheStateIterator< FactorWeightFst<A, F> >(fst) {}
};


// Specialization for FactorWeightFst.
template <class A, class F>
class ArcIterator< FactorWeightFst<A, F> >
    : public CacheArcIterator< FactorWeightFst<A, F> > {
 public:
  typedef typename A::StateId StateId;

  ArcIterator(const FactorWeightFst<A, F> &fst, StateId s)
      : CacheArcIterator< FactorWeightFst<A, F> >(fst, s) {
    if (!fst.impl_->HasArcs(s))
      fst.impl_->Expand(s);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ArcIterator);
};

template <class A, class F> inline
void FactorWeightFst<A, F>::InitStateIterator(StateIteratorData<A> *data) const
{
  data->base = new StateIterator< FactorWeightFst<A, F> >(*this);
}


}  // namespace fst

#endif // FST_LIB_FACTOR_WEIGHT_H__
