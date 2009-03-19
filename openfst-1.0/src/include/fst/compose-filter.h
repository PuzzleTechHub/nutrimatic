// compose-filter.h

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
// Classes for filtering the composition matches, e.g. for correct epsilon
// handling.

#ifndef FST_LIB_COMPOSE_FILTER_H__
#define FST_LIB_COMPOSE_FILTER_H__

#include <fst/fst.h>

namespace fst {


// COMPOSITION FILTER STATE - this represents the state of
// the composition filter. It has the form:
//
// class FilterState {
//  public:
//   // Required constructors
//   FilterState();
//   FilterState(const FilterState &f);
//   // An invalid filter state.
//   static const FilterState NoState();
//   // Maps state to integer for hashing.
//   size_t Hash() const;
//   // Equality of filter states.
//   bool operator==(const FilterState &f) const;
//   // Inequality of filter states.
//   bool operator!=(const FilterState &f) const;
//   // Assignment to filter states.
//   FilterState& operator=(const FilterState& f);
// };


class IntFilterState {
 public:
  IntFilterState() : state_(kNoStateId) { }
  IntFilterState(int s) : state_(s) { }

  static const IntFilterState NoState() { return IntFilterState(kNoStateId); }

  size_t Hash() const { return static_cast<size_t>(state_); }

  bool operator==(const IntFilterState &f) const {
    return state_ == f.state_;
  }

  bool operator!=(const IntFilterState &f) const {
    return state_ != f.state_;
  }

  int GetState() { return state_; }

  void SetState(int state)  { state_ = state; }

private:
  int state_;
};


// COMPOSITION FILTERS - these determine which matches are allowed to
// proceed. The filter's state is represented the type
// ComposeFilter::FilterState. The basic filters handle correct epsilon
// matching.  Their interface is:
//
// template <class A>
// class ComposeFilter {
//  public:
//   typedef A Arc;
//   typedef ... FilterState;
//
//   // Required constructor.
//   ComposeFilter(const Fst<A> &fst1, const Fst<A> &fst2);
//   // Return start state of filter.
//   // FilterState Start() const;
//   // Specifies current composition state.
//   void SetState(StateId s1, StateId s2, const FilterState &f)
//
//   // Apply filter at current composition state to these transitions.
//   // If an arc label is kNolabel, then that side does not consume a symbol.
//   // Returns the new filter state or if disallowed, FilterState::NoState();
//   // The filter is permitted to modify its inputs, e.g. for optimizations.
//   FilterState FilterArc(A *arc1, A *arc2) const;

//   // Apply filter at current composition state to these final weights
//   // (cf. superfinal transitions). The filter may modify its inputs,
//   // e.g. for optimizations.
//   void FilterFinal(Weight *final1, Weight *final2) const;

// };

// This filter requires epsilons on FST1 to be read first at a state.
template <class A>
class SequenceComposeFilter {
 public:
  typedef A Arc;
  typedef IntFilterState FilterState;

  typedef typename A::StateId StateId;
  typedef typename A::Label Label;
  typedef typename A::Weight Weight;

  SequenceComposeFilter(const Fst<A> &fst1, const Fst<A> &fst2)
      : fst1_(fst1.Copy()),
        fst2_(fst2.Copy()),
        s1_(kNoStateId),
        s2_(kNoStateId),
        f_(kNoStateId) {}

  SequenceComposeFilter(const SequenceComposeFilter<A> &filter)
      : fst1_(filter.fst1_->Copy()),
        fst2_(filter.fst2_->Copy()),
        s1_(kNoStateId),
        s2_(kNoStateId),
        f_(kNoStateId) {}

  ~SequenceComposeFilter() {
    delete fst1_;
    delete fst2_;
  }

  FilterState Start() const { return FilterState(0); }

  void SetState(StateId s1, StateId s2, const FilterState &f) {
    if (s1_ == s1 && s2_ == s2 && f == f_)
      return;
    s1_ = s1;
    s2_ = s2;
    f_ = f;
    size_t na1 = fst1_->NumArcs(s1);
    size_t ne1 = fst1_->NumOutputEpsilons(s1);
    bool fin1 = fst1_->Final(s1) != Weight::Zero();
    alleps1_ = na1 == ne1 && !fin1;
    noeps1_ = ne1 == 0;
  }

  FilterState FilterArc(A *arc1, A *arc2) const {
    if (arc1->olabel == kNoLabel)
      return  alleps1_ ? kNoLabel : noeps1_ ? FilterState(0) : FilterState(1);
    else if (arc2->ilabel == kNoLabel)
      return f_ != FilterState(0) ? FilterState::NoState() : FilterState(0);
    else
      return arc1->olabel == 0 ? FilterState::NoState() : FilterState(0);
  }

  void FilterFinal(Weight *, Weight *) const {}

 private:
  const Fst<A> *fst1_;
  const Fst<A> *fst2_;
  StateId s1_;     // Current fst1_ state;
  StateId s2_;     // Current fst2_ state;
  FilterState f_;  // Current filter state
  bool alleps1_;   // Only epsilons (and non-final) leaving s1_?
  bool noeps1_;    // No epsilons leaving s1_?

  void operator=(const SequenceComposeFilter<A> &);  // Disallow
};


// This filter requires epsilons on FST2 to be read first at a state.
template <class A>
class AltSequenceComposeFilter {
 public:
  typedef A Arc;
  typedef IntFilterState FilterState;

  typedef typename A::StateId StateId;
  typedef typename A::Label Label;
  typedef typename A::Weight Weight;


  AltSequenceComposeFilter(const Fst<A> &fst1, const Fst<A> &fst2)
      : fst1_(fst1.Copy()),
        fst2_(fst2.Copy()),
        s1_(kNoStateId),
        s2_(kNoStateId),
        f_(kNoStateId) {}

  AltSequenceComposeFilter(const AltSequenceComposeFilter<A> &filter)
      : fst1_(filter.fst1_->Copy()),
        fst2_(filter.fst2_->Copy()),
        s1_(kNoStateId),
        s2_(kNoStateId),
        f_(kNoStateId) {}

  ~AltSequenceComposeFilter() {
    delete fst1_;
    delete fst2_;
  }

  FilterState Start() const { return FilterState(0); }

  void SetState(StateId s1, StateId s2, const FilterState &f) {
    if (s1_ == s1 && s2_ == s2 && f == f_)
      return;
    s1_ = s1;
    s2_ = s2;
    f_ = f;
    size_t na2 = fst2_->NumArcs(s2);
    size_t ne2 = fst2_->NumInputEpsilons(s2);
    bool fin2 = fst2_->Final(s2) != Weight::Zero();
    alleps2_ = na2 == ne2 && !fin2;
    noeps2_ = ne2 == 0;
  }

  FilterState FilterArc( A *arc1, A *arc2) const {
    if (arc2->ilabel == kNoLabel)
      return alleps2_ ? kNoStateId : noeps2_ ? FilterState(0) : FilterState(1);
    else if (arc1->olabel == kNoLabel)
      return f_ == FilterState(1) ? FilterState::NoState() : FilterState(0);
    else
      return arc1->olabel == 0 ? FilterState::NoState() : FilterState(0);
  }

  void FilterFinal(Weight *, Weight *) const {}

 private:
  const Fst<A> *fst1_;
  const Fst<A> *fst2_;
  StateId s1_;     // Current fst1_ state;
  StateId s2_;     // Current fst2_ state;
  FilterState f_;  // Current filter state
  bool alleps2_;   // Only epsilons (and non-final) leaving s2_?
  bool noeps2_;    // No epsilons leaving s2_?

  void operator=(const AltSequenceComposeFilter<A> &);  // Disallow
};


// This filter enforces the matching of epsilons on FST1 with epsilons on FST2
// whenever possible.
template <class A>
class MatchComposeFilter {
 public:
  typedef A Arc;
  typedef IntFilterState FilterState;

  typedef typename A::StateId StateId;
  typedef typename A::Label Label;
  typedef typename A::Weight Weight;

  MatchComposeFilter(const Fst<A> &fst1, const Fst<A> &fst2)
      : fst1_(fst1.Copy()),
        fst2_(fst2.Copy()),
        s1_(kNoStateId),
        s2_(kNoStateId),
        f_(kNoStateId) {}

  MatchComposeFilter(const MatchComposeFilter<A> &filter)
      : fst1_(filter.fst1_->Copy()),
        fst2_(filter.fst2_->Copy()),
        s1_(kNoStateId),
        s2_(kNoStateId),
        f_(kNoStateId) {}

  ~MatchComposeFilter() {
    delete fst1_;
    delete fst2_;
  }

  FilterState Start() const { return FilterState(0); }

  void SetState(StateId s1, StateId s2, const FilterState &f) {
    if (s1_ == s1 && s2_ == s2 && f == f_)
      return;
    s1_ = s1;
    s2_ = s2;
    f_ = f;
    size_t na1 = fst1_->NumArcs(s1);
    size_t ne1 = fst1_->NumOutputEpsilons(s1);
    bool f1 = fst1_->Final(s1) != Weight::Zero();
    alleps1_ = na1 == ne1 && !f1;
    noeps1_ = ne1 == 0;
    size_t na2 = fst2_->NumArcs(s2);
    size_t ne2 = fst2_->NumInputEpsilons(s2);
    bool f2 = fst2_->Final(s2) != Weight::Zero();
    alleps2_ = na2 == ne2 && !f2;
    noeps2_ = ne2 == 0;
  }

  FilterState FilterArc(A *arc1, A *arc2) const {
    if (arc2->ilabel == kNoLabel)  // Epsilon on Fst1
      return f_ == FilterState(0) ?
          (noeps2_ ? FilterState(0) :
           (alleps2_ ? FilterState::NoState(): FilterState(1))) :
          (f_ == FilterState(1) ? FilterState(1) : FilterState::NoState());
    else if (arc1->olabel == kNoLabel)  // Epsilon on Fst2
      return f_ == FilterState(0) ?
          (noeps1_ ? FilterState(0) :
           (alleps1_ ? kNoStateId : FilterState(2))) :
          (f_ == FilterState(2) ? FilterState(2) : FilterState::NoState());
    else if (arc1->olabel == 0)  // Epsilon on both
      return f_ == FilterState(0) ? FilterState(0) : FilterState::NoState();
    else  // Both are non-epsilons
      return FilterState(0);
  }

  void FilterFinal(Weight *, Weight *) const {}

 private:
  const Fst<A> *fst1_;
  const Fst<A> *fst2_;
  StateId s1_;     // Current fst1_ state;
  StateId s2_;     // Current fst2_ state;
  FilterState f_;           // Current filter state ID
  bool alleps1_, alleps2_;  // Only epsilons (and non-final) leaving s1, s2?
  bool noeps1_, noeps2_;    // No epsilons leaving s1, s2?

  void operator=(const MatchComposeFilter<A> &);  // Disallow
};

}  // namespace fst


#endif  // FST_LIB_COMPOSE_FILTER_H__
