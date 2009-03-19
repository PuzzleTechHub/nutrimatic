// determinize.h


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
// Functions and classes to determinize an FST.

#ifndef FST_LIB_DETERMINIZE_H__
#define FST_LIB_DETERMINIZE_H__

#include <algorithm>
#include <tr1/unordered_map>
using std::tr1::unordered_map;
#include <map>
#include <fst/slist.h>
#include <string>
#include <vector>

#include <fst/cache.h>
#include <fst/factor-weight.h>
#include <fst/map.h>
#include <fst/prune.h>
#include <fst/test-properties.h>

namespace fst {

//
// COMMON DIVISORS - these are used in determinization to compute
// the transition weights. In the simplest case, it is just the same
// as the semiring Plus(). However, other choices permit more efficient
// determinization when the output contains strings.
//

// The default common divisor uses the semiring Plus.
template <class W>
class DefaultCommonDivisor {
 public:
  typedef W Weight;

  W operator()(const W &w1, const W &w2) const { return Plus(w1, w2); }
};


// The label common divisor for a (left) string semiring selects a
// single letter common prefix or the empty string. This is used in
// the determinization of output strings so that at most a single
// letter will appear in the output of a transtion.
template <typename L, StringType S>
class LabelCommonDivisor {
 public:
  typedef StringWeight<L, S> Weight;

  Weight operator()(const Weight &w1, const Weight &w2) const {
    StringWeightIterator<L, S> iter1(w1);
    StringWeightIterator<L, S> iter2(w2);

    if (!(StringWeight<L, S>::Properties() & kLeftSemiring))
      LOG(FATAL) << "LabelCommonDivisor: Weight needs to be left semiring";

    if (w1.Size() == 0 || w2.Size() == 0)
      return Weight::One();
    else if (w1 == Weight::Zero())
      return Weight(iter2.Value());
    else if (w2 == Weight::Zero())
      return Weight(iter1.Value());
    else if (iter1.Value() == iter2.Value())
      return Weight(iter1.Value());
    else
      return Weight::One();
  }
};


// The gallic common divisor uses the label common divisor on the
// string component and the template argument D common divisor on the
// weight component, which defaults to the default common divisor.
template <class L, class W, StringType S, class D = DefaultCommonDivisor<W> >
class GallicCommonDivisor {
 public:
  typedef GallicWeight<L, W, S> Weight;

  Weight operator()(const Weight &w1, const Weight &w2) const {
    return Weight(label_common_divisor_(w1.Value1(), w2.Value1()),
                  weight_common_divisor_(w1.Value2(), w2.Value2()));
  }

 private:
  LabelCommonDivisor<L, S> label_common_divisor_;
  D weight_common_divisor_;
};

// Options for finite-state transducer determinization.
template <class Arc>
struct DeterminizeFstOptions : CacheOptions {
  typedef typename Arc::Label Label;
  float delta;                // Quantization delta for subset weights
  Label subsequential_label;  // Label used for residual final output
                              // when producing subsequential transducers.

  explicit DeterminizeFstOptions(const CacheOptions &opts,
                                 float del = kDelta,
                                 Label lab = 0)
      : CacheOptions(opts), delta(del), subsequential_label(lab) {}

  explicit DeterminizeFstOptions(float del = kDelta, Label lab = 0)
      : delta(del), subsequential_label(lab) {}
};


// Implementation of delayed DeterminizeFst. This base class is
// common to the variants that implement acceptor and transducer
// determinization.
template <class A>
class DeterminizeFstImplBase : public CacheImpl<A> {
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

  DeterminizeFstImplBase(const Fst<A> &fst,
                         const DeterminizeFstOptions<A> &opts)
      : CacheImpl<A>(opts), fst_(fst.Copy()) {
    SetType("determinize");
    uint64 props = fst.Properties(kFstProperties, false);
    SetProperties(DeterminizeProperties(props,
                                        opts.subsequential_label != 0),
                  kCopyProperties);

    SetInputSymbols(fst.InputSymbols());
    SetOutputSymbols(fst.OutputSymbols());
  }

  DeterminizeFstImplBase(const DeterminizeFstImplBase<A> &impl)
      : CacheImpl<A>(impl),
        fst_(impl.fst_->Copy(true)) {
    SetType("determinize");
    SetProperties(impl.Properties(), kCopyProperties);
    SetInputSymbols(impl.InputSymbols());
    SetOutputSymbols(impl.OutputSymbols());
  }

  virtual ~DeterminizeFstImplBase() { delete fst_; }

  virtual DeterminizeFstImplBase<A> *Copy() = 0;

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

  virtual StateId ComputeStart() = 0;

  virtual Weight ComputeFinal(StateId s) = 0;

 protected:
  const Fst<A> *fst_;            // Input Fst

  void operator=(const DeterminizeFstImplBase<A> &);  // disallow
};


// Implementation of delayed determinization for weighted acceptors.
// It is templated on the arc type A and the common divisor D.
template <class A, class D>
class DeterminizeFsaImpl : public DeterminizeFstImplBase<A> {
 public:
  using DeterminizeFstImplBase<A>::fst_;

  typedef typename A::Label Label;
  typedef typename A::Weight Weight;
  typedef typename A::StateId StateId;

  struct Element {
    Element() {}

    Element(StateId s, Weight w) : state_id(s), weight(w) {}

    StateId state_id;  // Input state Id
    Weight weight;     // Residual weight
  };
  typedef slist<Element> Subset;
  typedef map<Label, Subset*> LabelMap;

  DeterminizeFsaImpl(const Fst<A> &fst, D common_divisor,
                     const vector<Weight> *in_dist, vector<Weight> *out_dist,
                     const DeterminizeFstOptions<A> &opts)
      : DeterminizeFstImplBase<A>(fst, opts),
        delta_(opts.delta),
        in_dist_(in_dist),
        out_dist_(out_dist),
        common_divisor_(common_divisor),
        subset_hash_(0, SubsetKey(), SubsetEqual(&elements_)) {
    if (!fst.Properties(kAcceptor, true))
     LOG(FATAL)  << "DeterminizeFst: argument not an acceptor";
    if (!(Weight::Properties() & kLeftSemiring))
      LOG(FATAL) << "DeterminizeFst: Weight needs to be left distributive: "
                 << Weight::Type();
    if (out_dist_)
      out_dist_->clear();
  }

  DeterminizeFsaImpl(const DeterminizeFsaImpl<A, D> &impl)
      : DeterminizeFstImplBase<A>(impl),
        delta_(impl.delta_),
        in_dist_(impl.in_dist_),
        out_dist_(impl.out_dist_),
        common_divisor_(impl.common_divisor_),
        subset_hash_(0, SubsetKey(), SubsetEqual(&elements_)) {
    if (out_dist_)
      LOG(FATAL) << "DeterminizeFsaImpl: cannot copy with out_dist vector";
  }

  virtual ~DeterminizeFsaImpl() {
    for (int i = 0; i < subsets_.size(); ++i)
      delete subsets_[i];
  }

  virtual DeterminizeFsaImpl<A, D> *Copy() {
    return new DeterminizeFsaImpl<A, D>(*this);
  }

  virtual StateId ComputeStart() {
    StateId s = fst_->Start();
    if (s == kNoStateId)
      return kNoStateId;
    Element element(s, Weight::One());
    Subset *subset = new Subset;
    subset->push_front(element);
    return FindState(subset);
  }



  virtual Weight ComputeFinal(StateId s) {
    Subset *subset = subsets_[s];
    Weight final = Weight::Zero();
    for (typename Subset::iterator siter = subset->begin();
         siter != subset->end();
         ++siter) {
      Element &element = *siter;
      final = Plus(final, Times(element.weight,
                                fst_->Final(element.state_id)));
      }
    return final;
  }

  // Finds the state corresponding to a subset. Only creates a new state
  // if the subset is not found in the subset hash. FindState takes
  // ownership of the subset argument (so that it doesn't have to copy it
  // if it creates a new state).
  //
  // The method exploits the following device: all pairs stored in the
  // associative container subset_hash_ are of the form (subset,
  // id(subset) + 1), i.e. subset_hash_[subset] > 0 if subset has been
  // stored previously. For unassigned subsets, the call to
  // subset_hash_[subset] creates a new pair (subset, 0). As a result,
  // subset_hash_[subset] == 0 iff subset is new.
  StateId FindState(Subset *subset) {
    StateId &assoc_value = subset_hash_[subset];
    if (assoc_value == 0) {    // subset wasn't present; create new state
      StateId s = CreateState(subset);
      assoc_value = s + 1;
      return  s;
    } else {
      delete subset;
      return assoc_value - 1;  // NB: assoc_value = ID + 1
    }
  }

  StateId CreateState(Subset *subset) {
    StateId s = subsets_.size();
    subsets_.push_back(subset);
    if (in_dist_)
      out_dist_->push_back(ComputeDistance(subset));
    return s;
  }

  // Compute distance from a state to the final states in the DFA
  // given the distances in the NFA.
  Weight ComputeDistance(const Subset *subset) {
    Weight outd = Weight::Zero();
    for (typename Subset::const_iterator siter = subset->begin();
         siter != subset->end(); ++siter) {
      const Element &element = *siter;
      Weight ind = element.state_id < in_dist_->size() ?
          (*in_dist_)[element.state_id] : Weight::Zero();
      outd = Plus(outd, Times(element.weight, ind));
    }
    return outd;
  }

  // Computes the outgoing transitions from a state, creating new destination
  // states as needed.
  virtual void Expand(StateId s) {

    LabelMap label_map;
    LabelSubsets(s, &label_map);

    for (typename LabelMap::iterator liter = label_map.begin();
         liter != label_map.end();
         ++liter)
      AddArc(s, liter->first, liter->second);
    SetArcs(s);
  }

 private:
  // Constructs destination subsets per label. At return, subset
  // element weights include the input automaton label weights and the
  // subsets may contain duplicate states.
  void LabelSubsets(StateId s, LabelMap *label_map) {
    Subset *src_subset = subsets_[s];

    for (typename Subset::iterator siter = src_subset->begin();
         siter != src_subset->end();
         ++siter) {
      Element &src_element = *siter;
      for (ArcIterator< Fst<A> > aiter(*fst_, src_element.state_id);
           !aiter.Done();
           aiter.Next()) {
        const A &arc = aiter.Value();
        Element dest_element(arc.nextstate,
                             Times(src_element.weight, arc.weight));
        Subset* &dest_subset = (*label_map)[arc.ilabel];
        if (dest_subset == 0)
          dest_subset = new Subset;
        dest_subset->push_front(dest_element);
      }
    }
  }

  // Adds an arc from state S to the destination state associated
  // with subset DEST_SUBSET (as created by LabelSubsets).
  void AddArc(StateId s, Label label, Subset *dest_subset) {
    A arc;
    arc.ilabel = label;
    arc.olabel = label;
    arc.weight = Weight::Zero();

    typename Subset::iterator oiter;
    for (typename Subset::iterator diter = dest_subset->begin();
         diter != dest_subset->end();) {
      Element &dest_element = *diter;
      // Computes label weight.
      arc.weight = common_divisor_(arc.weight, dest_element.weight);

      while (elements_.size() <= dest_element.state_id)
        elements_.push_back(0);
      Element *matching_element = elements_[dest_element.state_id];
      if (matching_element) {
        // Found duplicate state: sums state weight and deletes dup.
        matching_element->weight = Plus(matching_element->weight,
                                        dest_element.weight);
        ++diter;
        dest_subset->erase_after(oiter);
      } else {
        // Saves element so we can check for duplicate for this state.
        elements_[dest_element.state_id] = &dest_element;
        oiter = diter;
        ++diter;
      }
    }

    // Divides out label weight from destination subset elements.
    // Quantizes to ensure comparisons are effective.
    // Clears element vector.
    for (typename Subset::iterator diter = dest_subset->begin();
         diter != dest_subset->end();
         ++diter) {
      Element &dest_element = *diter;
      dest_element.weight = Divide(dest_element.weight, arc.weight,
                                   DIVIDE_LEFT);
      dest_element.weight = dest_element.weight.Quantize(delta_);
      elements_[dest_element.state_id] = 0;
    }

    arc.nextstate = FindState(dest_subset);
    CacheImpl<A>::AddArc(s, arc);
  }

  // Comparison object for hashing Subset(s). Subsets are not sorted in this
  // implementation, so ordering must not be assumed in the equivalence
  // test.
  class SubsetEqual {
   public:
    // Constructor takes vector needed to check equality. See immediately
    // below for constraints on it.
    explicit SubsetEqual(vector<Element *> *elements)
        : elements_(elements) {}

    // At each call to operator(), the elements_ vector should contain
    // only NULLs. When this operator returns, elements_ will still
    // have this property.
    bool operator()(Subset* subset1, Subset* subset2) const {
        if (subset1->size() != subset2->size())
          return false;

      // Loads first subset elements in element vector.
      for (typename Subset::iterator iter1 = subset1->begin();
           iter1 != subset1->end();
           ++iter1) {
        Element &element1 = *iter1;
        while (elements_->size() <= element1.state_id)
          elements_->push_back(0);
        (*elements_)[element1.state_id] = &element1;
      }

      // Checks second subset matches first via element vector.
      for (typename Subset::iterator iter2 = subset2->begin();
           iter2 != subset2->end();
           ++iter2) {
        Element &element2 = *iter2;
        while (elements_->size() <= element2.state_id)
          elements_->push_back(0);
        Element *element1 = (*elements_)[element2.state_id];
        if (!element1 || element1->weight != element2.weight) {
          // Mismatch found. Resets element vector before returning false.
          for (typename Subset::iterator iter1 = subset1->begin();
               iter1 != subset1->end();
               ++iter1)
            (*elements_)[iter1->state_id] = 0;
          return false;
        } else {
          (*elements_)[element2.state_id] = 0;  // Clears entry
        }
      }
      return true;
    }
   private:
    vector<Element *> *elements_;
  };

  // Hash function for Subset to Fst states. Subset elements are not
  // sorted in this implementation, so the hash must be invariant
  // under subset reordering.
  class SubsetKey {
   public:
    size_t operator()(const Subset* subset) const {
      size_t hash = 0;
      for (typename Subset::const_iterator iter = subset->begin();
           iter != subset->end();
           ++iter) {
        const Element &element = *iter;
        int lshift = element.state_id % kPrime;
        int rshift = sizeof(size_t) - lshift;
        hash ^= element.state_id << lshift ^
                element.state_id >> rshift ^
                element.weight.Hash();
      }
      return hash;
    }

   private:
    static const int kPrime = sizeof(size_t) == 8 ? 23 : 13;
  };

  float delta_;                    // Quantization delta for subset weights
  const vector<Weight> *in_dist_;  // Distance to final NFA states
  vector<Weight> *out_dist_;       // Distance to final DFA states

  D common_divisor_;

  // Used to test equivalence of subsets.
  vector<Element *> elements_;

  // Maps from StateId to Subset.
  vector<Subset *> subsets_;

  // Hashes from Subset to its StateId in the output automaton.
  typedef unordered_map<Subset *, StateId, SubsetKey, SubsetEqual>
  SubsetHash;

  // Hashes from Label to Subsets corr. to destination states of current state.
  SubsetHash subset_hash_;

  void operator=(const DeterminizeFsaImpl<A, D> &);  // disallow
};


// Implementation of delayed determinization for transducers.
// Transducer determinization is implemented by mapping the input to
// the Gallic semiring as an acceptor whose weights contain the output
// strings and using acceptor determinization above to determinize
// that acceptor.
template <class A, StringType S>
class DeterminizeFstImpl : public DeterminizeFstImplBase<A> {
 public:
  typedef typename A::Label Label;
  typedef typename A::Weight Weight;
  typedef typename A::StateId StateId;

  typedef ToGallicMapper<A, S> ToMapper;
  typedef FromGallicMapper<A, S> FromMapper;

  typedef typename ToMapper::ToArc ToArc;
  typedef MapFst<A, ToArc, ToMapper> ToFst;
  typedef MapFst<ToArc, A, FromMapper> FromFst;

  typedef GallicCommonDivisor<Label, Weight, S> CommonDivisor;
  typedef GallicFactor<Label, Weight, S> FactorIterator;

  // Defined after DeterminizeFst since it calls it.
  DeterminizeFstImpl(const Fst<A> &fst, const DeterminizeFstOptions<A> &opts);

  DeterminizeFstImpl(const DeterminizeFstImpl<A, S> &impl)
      : DeterminizeFstImplBase<A>(impl),
        from_fst_(impl.from_fst_->Copy()) {
  }

  ~DeterminizeFstImpl() { delete from_fst_; }

  virtual DeterminizeFstImpl<A, S> *Copy() {
    return new DeterminizeFstImpl<A, S>(*this);
  }

  virtual StateId ComputeStart() { return from_fst_->Start(); }

  virtual Weight ComputeFinal(StateId s) { return from_fst_->Final(s); }

  virtual void Expand(StateId s) {
    for (ArcIterator<FromFst> aiter(*from_fst_, s);
         !aiter.Done();
         aiter.Next())
      CacheImpl<A>::AddArc(s, aiter.Value());
    CacheImpl<A>::SetArcs(s);
  }

 private:
  FromFst *from_fst_;

  void operator=(const DeterminizeFstImpl<A, S> &);  // disallow
};


// Determinizes a weighted transducer. This version is a delayed
// Fst. The result will be an equivalent FST that has the property
// that no state has two transitions with the same input label.
// For this algorithm, epsilon transitions are treated as regular
// symbols (cf. RmEpsilon).
//
// The transducer must be functional. The weights must be (weakly)
// left divisible (valid for TropicalWeight and LogWeight).
//
// Complexity:
// - Determinizable: exponential (polynomial in the size of the output)
// - Non-determinizable) does not terminate
//
// The determinizable automata include all unweighted and all acyclic input.
//
// References:
// - Mehryar Mohri, "Finite-State Transducers in Language and Speech
//   Processing". Computational Linguistics, 23:2, 1997.
template <class A>
class DeterminizeFst : public Fst<A> {
 public:
  friend class ArcIterator< DeterminizeFst<A> >;
  friend class CacheStateIterator< DeterminizeFst<A> >;
  friend class CacheArcIterator< DeterminizeFst<A> >;
  template <class B, StringType S> friend class DeterminizeFstImpl;

  typedef A Arc;
  typedef typename A::Weight Weight;
  typedef typename A::StateId StateId;
  typedef typename A::Label Label;
  typedef CacheState<A> State;

  explicit DeterminizeFst(
      const Fst<A> &fst,
      const DeterminizeFstOptions<A> &opts = DeterminizeFstOptions<A>()) {
    if (fst.Properties(kAcceptor, true)) {
      // Calls implementation for acceptors.
      typedef DefaultCommonDivisor<Weight> D;
      impl_ = new DeterminizeFsaImpl<A, D>(fst, D(), 0, 0, opts);
    } else {
      // Calls implementation for transducers.
      impl_ = new DeterminizeFstImpl<A, STRING_LEFT_RESTRICT>(fst, opts);
    }
  }

  // This acceptor-only version additionally computes the distance to
  // final states in the output if provided with those distances for the
  // input. Useful for e.g. unique N-shortest paths.
  DeterminizeFst(
      const Fst<A> &fst,
      const vector<Weight> &in_dist, vector<Weight> *out_dist,
      const DeterminizeFstOptions<A> &opts = DeterminizeFstOptions<A>()) {
    if (!fst.Properties(kAcceptor, true))
      LOG(FATAL) << "DeterminizeFst:"
                 << " distance to final states computed for acceptors only";
    typedef DefaultCommonDivisor<Weight> D;
    impl_ = new DeterminizeFsaImpl<A, D>(fst, D(), &in_dist, out_dist, opts);
  }

  DeterminizeFst(const DeterminizeFst<A> &fst, bool reset = false) {
    if (reset) {
      impl_ = fst.impl_->Copy();
    } else {
      impl_ = fst.impl_;
      impl_->IncrRefCount();
    }
  }

  virtual ~DeterminizeFst() { if (!impl_->DecrRefCount()) delete impl_; }

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

  virtual DeterminizeFst<A> *Copy(bool reset = false) const {
    return new DeterminizeFst<A>(*this, reset);
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
  DeterminizeFstImplBase<A> *Impl() { return impl_; }

 private:
  // This private version is for passing the common divisor to
  // FSA determinization.
  template <class D>
  DeterminizeFst(const Fst<A> &fst, const D &common_div,
                 const DeterminizeFstOptions<A> &opts)
      :  impl_(new DeterminizeFsaImpl<A, D>(fst, common_div, 0, 0, opts)) {}

  DeterminizeFstImplBase<A> *impl_;

  void operator=(const DeterminizeFst<A> &fst);  // Disallow
};


template <class A, StringType S>
DeterminizeFstImpl<A, S>::DeterminizeFstImpl(
    const Fst<A> &fst, const DeterminizeFstOptions<A> &opts)
    : DeterminizeFstImplBase<A>(fst, opts) {

  // Mapper to an acceptor.
  ToFst to_fst(fst, ToMapper());

  // Determinize acceptor.
  // This recursive call terminates since it passes the common divisor
  // to a private constructor.
  DeterminizeFstOptions<ToArc> dopts(opts, opts.delta);
  DeterminizeFst<ToArc> det_fsa(to_fst, CommonDivisor(), dopts);

  // Mapper back to transducer.
  FactorWeightOptions<ToArc> fopts(CacheOptions(true, 0), opts.delta,
                                   kFactorFinalWeights,
                                   opts.subsequential_label,
                                   opts.subsequential_label);
  FactorWeightFst<ToArc, FactorIterator> factored_fst(det_fsa, fopts);
  from_fst_ = new FromFst(factored_fst, FromMapper(opts.subsequential_label));
}


// Specialization for DeterminizeFst.
template <class A>
class StateIterator< DeterminizeFst<A> >
    : public CacheStateIterator< DeterminizeFst<A> > {
 public:
  explicit StateIterator(const DeterminizeFst<A> &fst)
      : CacheStateIterator< DeterminizeFst<A> >(fst) {}
};


// Specialization for DeterminizeFst.
template <class A>
class ArcIterator< DeterminizeFst<A> >
    : public CacheArcIterator< DeterminizeFst<A> > {
 public:
  typedef typename A::StateId StateId;

  ArcIterator(const DeterminizeFst<A> &fst, StateId s)
      : CacheArcIterator< DeterminizeFst<A> >(fst, s) {
    if (!fst.impl_->HasArcs(s))
      fst.impl_->Expand(s);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ArcIterator);
};


template <class A> inline
void DeterminizeFst<A>::InitStateIterator(StateIteratorData<A> *data) const
{
  data->base = new StateIterator< DeterminizeFst<A> >(*this);
}


// Useful aliases when using StdArc.
typedef DeterminizeFst<StdArc> StdDeterminizeFst;


template <class Arc>
struct DeterminizeOptions {
  typedef typename Arc::StateId StateId;
  typedef typename Arc::Weight Weight;
  typedef typename Arc::Label Label;

  float delta;                // Quantization delta for subset weights.
  Weight weight_threshold;    // Pruning weight threshold.
  StateId state_threshold;    // Pruning state threshold.
  Label subsequential_label;  // Label used for residual final output
  // when producing subsequential transducers.

  explicit DeterminizeOptions(float d = kDelta, Weight w = Weight::Zero(),
                              StateId n = kNoStateId, Label l = 0)
      : delta(d), weight_threshold(w), state_threshold(n),
        subsequential_label(l) {}
};


// Determinizes a weighted transducer.  This version writes the
// determinized Fst to an output MutableFst.  The result will be an
// equivalent FSt that has the property that no state has two
// transitions with the same input label.  For this algorithm, epsilon
// transitions are treated as regular symbols (cf. RmEpsilon).
//
// The transducer must be functional. The weights must be (weakly)
// left divisible (valid for TropicalWeight and LogWeight).
//
// Complexity:
// - Determinizable: exponential (polynomial in the size of the output)
// - Non-determinizable: does not terminate
//
// The determinizable automata include all unweighted and all acyclic input.
//
// References:
// - Mehryar Mohri, "Finite-State Transducers in Language and Speech
//   Processing". Computational Linguistics, 23:2, 1997.
template <class Arc>
void Determinize(const Fst<Arc> &ifst, MutableFst<Arc> *ofst,
             const DeterminizeOptions<Arc> &opts
                 = DeterminizeOptions<Arc>()) {
  typedef typename Arc::StateId StateId;
  typedef typename Arc::Weight Weight;

  DeterminizeFstOptions<Arc> nopts;
  nopts.delta = opts.delta;
  nopts.subsequential_label = opts.subsequential_label;

  nopts.gc_limit = 0;  // Cache only the last state for fastest copy.

  if (opts.weight_threshold != Weight::Zero() ||
      opts.state_threshold != kNoStateId) {
    if (ifst.Properties(kAcceptor, false)) {
      vector<Weight> idistance, odistance;
      ShortestDistance(ifst, &idistance, true);
      DeterminizeFst<Arc> dfst(ifst, idistance, &odistance, nopts);
      PruneOptions< Arc, AnyArcFilter<Arc> > popts(opts.weight_threshold,
                                                   opts.state_threshold,
                                                   AnyArcFilter<Arc>(),
                                                   &odistance);
      Prune(dfst, ofst, popts);
    } else {
      *ofst = DeterminizeFst<Arc>(ifst, nopts);
      Prune(ofst, opts.weight_threshold, opts.state_threshold);
    }
  } else {
    *ofst = DeterminizeFst<Arc>(ifst, nopts);
  }
}


}  // namespace fst

#endif  // FST_LIB_DETERMINIZE_H__
