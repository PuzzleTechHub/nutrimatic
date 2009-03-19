// matcher.h

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
// Classes to allow matching labels leaving FST states.

#ifndef FST_LIB_MATCHER_H__
#define FST_LIB_MATCHER_H__

#include <algorithm>

#include <fst/fst.h>

namespace fst {

// MATCHERS - these return iterators to requested labels at
// FST states. In the simplest form, these are just some associative
// map or search keyed on labels. More generally, they may
// implement matching special labels that represent sets of labels
// such as 'sigma' (all), 'rho' (rest), or 'phi' (fail).
// The Matcher interface is:
//
// template <class F>
// class Matcher {
//  public:
//   typedef F FST;
//   typedef F::Arc Arc;
//   typedef typename Arc::StateId StateId;
//   typedef typename Arc::Label Label;
//   typedef typename Arc::Weight Weight;
//
//   // Required constructors.
//   Matcher(const F &fst, MatchType type);
//   Matcher(const F &matcher);
//
//   // Returns the match type that can be provided (depending on
//   // compatibility of the input FST). It is either
//   // the requested match type, MATCH_NONE, or MATCH_UNKNOWN.
//   // If 'test' is false, a constant time test is performed, but
//   // MATCH_UNKNOWN may be returned. If 'test' is true,
//   // a definite answer is returned, but may involve more costly
//   // computation (e.g., visiting the Fst).
//   MatchType Type(bool test) const;

//   // Specifies the current state.
//   void SetState(StateId s);
//
//   // This finds matches to a label at the current state.
//   // Returns true if a match found. kNoLabel matches any
//   // 'non-consuming' transitions, e.g., epsilon transitions,
//   // which do not require a matching symbol.
//   bool Find(Label label);
//   // These iterate through any matches found:
//   bool Done() const;         // No more matches.
//   const A& Value() const;    // Current arc (when !Done)
//   void Next();               // Advance to next arc (when !Done)
//
//   // This specifies the known Fst properties as viewed from this
//   // mapper. It takes as argument the input Fst's known properties.
//   uint64 Properties(uint64 props) const;
// };


// Matcher interface, templated on the Arc definition; used
// for matcher specializations that are returned by the
// InitMatcher Fst method.
template <class A>
class MatcherBase {
 public:
  typedef A Arc;
  typedef typename A::StateId StateId;
  typedef typename A::Label Label;
  typedef typename A::Weight Weight;

  virtual ~MatcherBase() {}

  virtual MatcherBase<A> *Copy() const = 0;
  virtual MatchType Type(bool test) const = 0;
  void SetState(StateId s) { SetState_(s); }

  bool Find(Label label, MatcherData<A> *data = 0) {
    return Find_(label, data);
  }

  bool Done() const { return Done_(); }
  const A& Value() const { return Value_(); }
  void Next() { Next_(); }

  virtual uint64 Properties(uint64 props) const = 0;

 private:
  virtual void SetState_(StateId s) = 0;
  virtual bool Find_(Label label, MatcherData<A> *data) = 0;
  virtual bool Done_() const = 0;
  virtual const A& Value_() const  = 0;
  virtual void Next_()  = 0;
};


// A matcher that expects sorted labels on the side to be matched.
// If match_type == MATCH_INPUT, epsilons match the implicit self loop
// Arc(kNoLabel, 0, Weight::One(), current_state) as well as any
// actual epsilon transitions. If match_type == MATCH_OUTPUT, then
// Arc(0, kNoLabel, Weight::One(), current_state) is instead matched.
template <class F>
class SortedMatcher : public MatcherBase<typename F::Arc> {
 public:
  typedef F FST;
  typedef typename F::Arc Arc;
  typedef typename Arc::StateId StateId;
  typedef typename Arc::Label Label;
  typedef typename Arc::Weight Weight;

  // Labels >= binary_label will be searched for by binary search,
  // o.w. linear search is used.
  SortedMatcher(const F &fst, MatchType match_type,
                Label binary_label = 1)
      : fst_(fst.Copy()),
        aiter_(0),
        match_type_(match_type),
        binary_label_(binary_label),
        match_label_(kNoLabel),
        narcs_(0),
        loop_(kNoLabel, 0, Weight::One(), kNoStateId) {
    switch(match_type_) {
      case MATCH_INPUT:
      case MATCH_NONE:
        break;
      case MATCH_OUTPUT:
        swap(loop_.ilabel, loop_.olabel);
        break;
      default:
        LOG(FATAL) << "SortedMatcher: bad match type";
    }
  }

  SortedMatcher(const SortedMatcher<F> &matcher)
      : fst_(matcher.fst_->Copy()),
        aiter_(0),
        match_type_(matcher.match_type_),
        binary_label_(matcher.binary_label_),
        match_label_(kNoLabel),
        narcs_(0),
        loop_(matcher.loop_) {}

  virtual ~SortedMatcher() {
    if (aiter_)
      delete aiter_;
    delete fst_;
  }

  virtual SortedMatcher<F> *Copy() const {
    return new SortedMatcher<F>(*this);
  }

  virtual MatchType Type(bool test) const {
    if (match_type_ == MATCH_NONE)
      return match_type_;

    uint64 true_prop =  match_type_ == MATCH_INPUT ?
        kILabelSorted : kOLabelSorted;
    uint64 false_prop = match_type_ == MATCH_INPUT ?
        kNotILabelSorted : kNotOLabelSorted;
    uint64 props = fst_->Properties(true_prop | false_prop, test);

    if (props & true_prop)
      return match_type_;
    else if (props & false_prop)
      return MATCH_NONE;
    else
      return MATCH_UNKNOWN;
  }

  void SetState(StateId s) {
    if (match_type_ == MATCH_NONE)
        LOG(FATAL) << "SortedMatcher: bad match type";
    if (aiter_)
      delete aiter_;
    aiter_ = new ArcIterator<F>(*fst_, s);
    narcs_ = fst_->NumArcs(s);
    loop_.nextstate = s;
  }

  bool Find(Label match_label, MatcherData<Arc> *data = 0) {
    current_loop_ = match_label == 0;
    match_label_ = match_label == kNoLabel ? 0 : match_label;
    if (match_label_ >= binary_label_) {
      // Binary search for match.
      size_t low = 0;
      size_t high = narcs_;
      while (low < high) {
        size_t mid = (low + high) / 2;
        aiter_->Seek(mid);
        Label label = match_type_ == MATCH_INPUT ?
            aiter_->Value().ilabel : aiter_->Value().olabel;
        if (label > match_label_) {
          high = mid;
        } else if (label < match_label_) {
          low = mid + 1;
        } else {
          // find first matching label (when non-determinism)
          for (size_t i = mid; i > low; --i) {
            aiter_->Seek(i - 1);
            label = match_type_ == MATCH_INPUT ? aiter_->Value().ilabel :
                aiter_->Value().olabel;
            if (label != match_label_) {
              aiter_->Seek(i);
              return true;
            }
          }
          return true;
        }
       }
       return current_loop_;
    } else {
      // Linear search for match.
      for (aiter_->Reset(); !aiter_->Done(); aiter_->Next()) {
        Label label = match_type_ == MATCH_INPUT ?
            aiter_->Value().ilabel : aiter_->Value().olabel;
        if (label == match_label_)
          return true;
        if (label > match_label_)
          break;
      }
      return current_loop_;
    }
  }

  bool Done() const {
    if (current_loop_)
      return false;
    if (aiter_->Done())
      return true;
    Label label = match_type_ == MATCH_INPUT ?
        aiter_->Value().ilabel : aiter_->Value().olabel;
    return label != match_label_;
  }

  const Arc& Value() const {
    return current_loop_ ? loop_ : aiter_->Value();
  }

  void Next() {
    if (current_loop_)
      current_loop_ = false;
    else
      aiter_->Next();
  }

  virtual uint64 Properties(uint64 props) const { return props; }

 private:
  virtual void SetState_(StateId s) { SetState(s); }

  virtual bool Find_(Label label, MatcherData<Arc> *data = 0) {
    return Find(label, data);
  }

  virtual bool Done_() const { return Done(); }
  virtual const Arc& Value_() const { return Value(); }
  virtual void Next_() { Next(); }

  const F *fst_;
  ArcIterator<F> *aiter_;         // Iterator for current state
  MatchType match_type_;          // Type of match to perform
  Label binary_label_;            // Least label for binary search
  Label match_label_;             // Current label to be matched
  size_t narcs_;                 // Current state arc count
  Arc loop_;                      // For non-consuming symbols
  bool current_loop_;             // Current arc is the implicit loop

  void operator=(const SortedMatcher<F> &);  // Disallow
};


// For any requested label that doesn't match at a state, this matcher
// considers all transitions that match the label 'rho_label' (rho =
// 'rest').  Each such rho transition found is returned with the
// rho_label rewritten as the requested label (both sides if an
// acceptor, or if 'rewrite_both' is true and both input and output
// labels of the found transition are 'rho_label').  If 'rho_label' is
// kNoLabel, this special matching is not done.  RhoMatcher is
// templated itself on a matcher, which is used to perform the
// underlying matching.  By default, the underlying matcher is
// constructed by RhoMatcher.  The user can instead pass in this
// object; in that case, RhoMatcher takes its ownership.
template <class M>
class RhoMatcher {
 public:
  typedef typename M::FST F;
  typedef typename M::Arc Arc;
  typedef typename Arc::StateId StateId;
  typedef typename Arc::Label Label;
  typedef typename Arc::Weight Weight;

  RhoMatcher(const F &fst,
             MatchType match_type,
             Label rho_label = kNoLabel,
             bool rewrite_both = false,
             M *matcher = 0)
      : matcher_(matcher ? matcher : new M(fst, match_type)),
        match_type_(match_type),
        rho_label_(rho_label),
        rewrite_both_(rewrite_both ? true : fst.Properties(kAcceptor, true)) {
    if (match_type == MATCH_BOTH)
      LOG(FATAL) << "RhoMatcher: bad match type";
    if (rho_label == 0)
      LOG(FATAL) << "RhoMatcher: 0 cannot be used as rho_label";
  }

  RhoMatcher(const RhoMatcher<M> &matcher)
      : matcher_(new M(*matcher.matcher_)),
        match_type_(matcher.match_type_),
        rho_label_(matcher.rho_label_),
        rewrite_both_(matcher.rewrite_both_) {}

  ~RhoMatcher() {
    delete matcher_;
  }

  MatchType Type(bool test) const { return matcher_->Type(test); }

  void SetState(StateId s) {
    matcher_->SetState(s);
    has_rho_ = rho_label_ != kNoLabel;
  }

  bool Find(Label match_label) {
    if (match_label == rho_label_ && rho_label_ != kNoLabel) {
      LOG(FATAL) << "RhoMatcher::Find: bad label (rho)";
    }
    if (matcher_->Find(match_label)) {
      rho_match_ = kNoLabel;
      return true;
    } else if (has_rho_ && match_label != 0 && match_label != kNoLabel &&
               (has_rho_ = matcher_->Find(rho_label_))) {
      rho_match_ = match_label;
      return true;
    } else {
      return false;
    }
  }

  bool Done() const { return matcher_->Done(); }

  const Arc& Value() const {
    if (rho_match_ == kNoLabel) {
      return matcher_->Value();
    } else {
      rho_arc_ = matcher_->Value();
      if (rewrite_both_) {
        if (rho_arc_.ilabel == rho_label_)
          rho_arc_.ilabel = rho_match_;
        if (rho_arc_.olabel == rho_label_)
          rho_arc_.olabel = rho_match_;
      } else if (match_type_ == MATCH_INPUT) {
        rho_arc_.ilabel = rho_match_;
      } else {
        rho_arc_.olabel = rho_match_;
      }
      return rho_arc_;
    }
  }

  void Next() { matcher_->Next(); }

  uint64 Properties(uint64 props) const {
    if (match_type_ == MATCH_NONE)
      return props;
    else if (rewrite_both_)
      return props & ~(kIDeterministic | kNonIDeterministic |
                       kODeterministic | kNonODeterministic | kString);
    else //
      return props & ~kString;
  }

 private:
  M *matcher_;
  MatchType match_type_;  // Type of match requested
  Label rho_label_;       // Label that represents the rho transition
  bool rewrite_both_;     // Rewrite both sides when both are 'rho_label_'
  bool has_rho_;          // Are there possibly rhos at the current state?
  Label rho_match_;       // Current label that matches rho transition
  mutable Arc rho_arc_;   // Arc to return when rho match

  void operator=(const RhoMatcher<M> &);  // Disallow
};


// For any requested label, this matcher considers all transitions
// that match the label 'sigma_label' (sigma = "any"), and this in
// additions to transitions with the requested label.  Each such sigma
// transition found is returned with the sigma_label rewritten as the
// requested label (both sides if an acceptor, or if 'rewrite_both' is
// true and both input and output labels of the found transition are
// 'sigma_label').  If 'sigma_label' is kNoLabel, this special
// matching is not done.  SigmaMatcher is templated itself on a
// matcher, which is used to perform the underlying matching.  By
// default, the underlying matcher is constructed by SigmaMatcher.
// The user can instead pass in this object; in that case,
// SigmaMatcher takes its ownership.
template <class M>
class SigmaMatcher {
 public:
  typedef typename M::FST F;
  typedef typename M::Arc Arc;
  typedef typename Arc::StateId StateId;
  typedef typename Arc::Label Label;
  typedef typename Arc::Weight Weight;

  SigmaMatcher(const F &fst,
               MatchType match_type,
               Label sigma_label = kNoLabel,
               bool rewrite_both = false,
               M *matcher = 0)
      : matcher_(matcher ? matcher : new M(fst, match_type)),
        match_type_(match_type),
        sigma_label_(sigma_label),
        rewrite_both_(rewrite_both ? true :  fst.Properties(kAcceptor, true)) {
    if (match_type == MATCH_BOTH)
      LOG(FATAL) << "SigmaMatcher: bad match type";
    if (sigma_label == 0)
      LOG(FATAL) << "SigmaMatcher: 0 cannot be used as sigma_label";
  }

  SigmaMatcher(const SigmaMatcher<M> &matcher)
      : matcher_(new M(*matcher.matcher_)),
        match_type_(matcher.match_type_),
        sigma_label_(matcher.sigma_label_),
        rewrite_both_(matcher.rewrite_both_) {}

  ~SigmaMatcher() {
    delete matcher_;
  }

  MatchType Type(bool test) const { return matcher_->Type(test); }

  void SetState(StateId s) {
    matcher_->SetState(s);
    has_sigma_ =
        sigma_label_ != kNoLabel ? matcher_->Find(sigma_label_) : false;
  }

  bool Find(Label match_label) {
    match_label_ = match_label;
    if (match_label == sigma_label_ && sigma_label_ != kNoLabel) {
      LOG(FATAL) << "SigmaMatcher::Find: bad label (sigma)";
    }
    if (matcher_->Find(match_label)) {
      sigma_match_ = kNoLabel;
      return true;
    } else if (has_sigma_ && match_label != 0 && match_label != kNoLabel &&
               matcher_->Find(sigma_label_)) {
      sigma_match_ = match_label;
      return true;
    } else {
      return false;
    }
  }

  bool Done() const {
    return matcher_->Done();
  }

  const Arc& Value() const {
    if (sigma_match_ == kNoLabel) {
      return matcher_->Value();
    } else {
      sigma_arc_ = matcher_->Value();
      if (rewrite_both_) {
        if (sigma_arc_.ilabel == sigma_label_)
          sigma_arc_.ilabel = sigma_match_;
        if (sigma_arc_.olabel == sigma_label_)
          sigma_arc_.olabel = sigma_match_;
      } else if (match_type_ == MATCH_INPUT) {
        sigma_arc_.ilabel = sigma_match_;
      } else {
        sigma_arc_.olabel = sigma_match_;
      }
      return sigma_arc_;
    }
  }

  void Next() {
    matcher_->Next();
    if (matcher_->Done() && has_sigma_ && (sigma_match_ == kNoLabel) &&
        (match_label_ > 0)) {
      matcher_->Find(sigma_label_);
      sigma_match_ = match_label_;
    }
  }

  uint64 Properties(uint64 props) const {
    if (match_type_ == MATCH_NONE)
      return props;
    else if (rewrite_both_)
      return props & ~(kIDeterministic | kNonIDeterministic |
                       kODeterministic | kNonODeterministic | kString);
    else if (match_type_ == MATCH_INPUT)
      return props & ~(kIDeterministic | kString);
    else  // MATCH_OUTPUT
      return props & ~(kODeterministic | kString);
  }

private:
  M *matcher_;
  MatchType match_type_;   // Type of match requested
  Label sigma_label_;      // Label that represents the sigma transition
  bool rewrite_both_;      // Rewrite both sides when both are 'sigma_label_'
  bool has_sigma_;         // Are there sigmas at the current state?
  Label sigma_match_;      // Current label that matches sigma transition
  mutable Arc sigma_arc_;  // Arc to return when sigma match
  Label match_label_;      // Label being matched

  void operator=(const SigmaMatcher<M> &);  // disallow
};


// For any requested label that doesn't match at a state, this matcher
// considers the unique transition that match the label 'phi_label'
// (phi = 'fail'), and recursively look for a match at its
// destination.  When 'phi_loop' is true, if no match is found but a
// phi self-loop is found, then the phi transition found is returned
// with the phi_label rewritten as the requested label (both sides if
// an acceptor, or if 'rewrite_both' is true and both input and output
// labels of the found transition are 'phi_label').  If 'phi_label' is
// kNoLabel, this special matching is not done.  PhiMatcher is
// templated itself on a matcher, which is used to perform the
// underlying matching.  By default, the underlying matcher is
// constructed by PhiMatcher. The user can instead pass in this
// object; in that case, PhiMatcher takes its ownership.
template <class M>
class PhiMatcher {
 public:
  typedef typename M::FST F;
  typedef typename M::Arc Arc;
  typedef typename Arc::StateId StateId;
  typedef typename Arc::Label Label;
  typedef typename Arc::Weight Weight;

  PhiMatcher(const F &fst,
             MatchType match_type,
             Label phi_label = kNoLabel,
             bool phi_loop = true,
             bool rewrite_both = false,
             M *matcher = 0)
      : matcher_(matcher ? matcher : new M(fst, match_type)),
        match_type_(match_type),
        phi_label_(phi_label),
        rewrite_both_(rewrite_both ? true : fst.Properties(kAcceptor, true)),
        state_(kNoStateId),
        phi_loop_(phi_loop) {
    if (match_type == MATCH_BOTH)
      LOG(FATAL) << "PhiMatcher: bad match type";
    if (phi_label == 0)
      LOG(FATAL) << "PhiMatcher: 0 cannot be used as phi_label";
   }

  PhiMatcher(const PhiMatcher<M> &matcher)
      : matcher_(new M(*matcher.matcher_)),
        match_type_(matcher.match_type_),
        phi_label_(matcher.phi_label_),
        rewrite_both_(matcher.rewrite_both_),
        state_(kNoStateId),
        phi_loop_(matcher.phi_loop_) {}

  ~PhiMatcher() {
    delete matcher_;
  }

  MatchType Type(bool test) const { return matcher_->Type(test); }

  void SetState(StateId s) {
    matcher_->SetState(s);
    state_ = s;
    has_phi_ = phi_label_ != kNoLabel;
  }

  bool Find(Label match_label) {
    if (match_label == phi_label_ && phi_label_ != kNoLabel) {
      LOG(FATAL) << "PhiMatcher::Find: bad label (phi)";
    }
    matcher_->SetState(state_);
    phi_match_ = kNoLabel;
    phi_weight_ = Weight::One();
    if (!has_phi_ || match_label == 0 || match_label == kNoLabel)
      return matcher_->Find(match_label);
    StateId state = state_;
    while (!matcher_->Find(match_label)) {
      if (!matcher_->Find(phi_label_))
        return false;
      if (phi_loop_ && matcher_->Value().nextstate == state) {
        phi_match_ = match_label;
        return true;
      }
      phi_weight_ = Times(phi_weight_, matcher_->Value().weight);
      state = matcher_->Value().nextstate;
      matcher_->SetState(state);
    }
    return true;
  }

  bool Done() const { return matcher_->Done(); }

  const Arc& Value() const {
    if ((phi_match_ == kNoLabel) && (phi_weight_ == Weight::One())) {
      return matcher_->Value();
    } else {
      phi_arc_ = matcher_->Value();
      phi_arc_.weight = Times(phi_weight_, phi_arc_.weight);
      if (phi_match_ != kNoLabel) {
        if (rewrite_both_) {
          if (phi_arc_.ilabel == phi_label_)
            phi_arc_.ilabel = phi_match_;
          if (phi_arc_.olabel == phi_label_)
            phi_arc_.olabel = phi_match_;
        } else if (match_type_ == MATCH_INPUT) {
          phi_arc_.ilabel = phi_match_;
        } else {
          phi_arc_.olabel = phi_match_;
        }
      }
      return phi_arc_;
    }
  }

  void Next() { matcher_->Next(); }


  uint64 Properties(uint64 props) const {
    if (match_type_ == MATCH_NONE)
      return props;
    else if (rewrite_both_)
      return props & ~(kIDeterministic | kNonIDeterministic |
                       kODeterministic | kNonODeterministic | kString);
    else //
      return props & ~kString;
  }

private:
  M *matcher_;
  MatchType match_type_;  // Type of match requested
  Label phi_label_;       // Label that represents the phi transition
  bool rewrite_both_;     // Rewrite both sides when both are 'phi_label_'
  bool has_phi_;          // Are there possibly phis at the current state?
  Label phi_match_;       // Current label that matches phi loop
  mutable Arc phi_arc_;   // Arc to return
  StateId state_;         // State where looking for matches
  Weight phi_weight_;     // Product of the weights of phi transitions taken
  bool phi_loop_;         // When true, phi self-loop are allowed and treated
                          // as rho (required for Aho-Corasick)

  void operator=(const PhiMatcher<M> &);  // disallow
};


// Generic matcher, templated on the FST definition
// - a wrapper around pointer to specific one.
// Here is a typical use: \code
//   Matcher<StdFst> matcher(fst, MATCH_INPUT);
//   matcher.SetState(state);
//   if (matcher.Find(label))
//     for (; !matcher.Done(); matcher.Next()) {
//       StdArc &arc = matcher.Value();
//       ...
//     } \endcode
template <class F>
class Matcher {
 public:
  typedef F FST;
  typedef typename F::Arc Arc;
  typedef typename Arc::StateId StateId;
  typedef typename Arc::Label Label;
  typedef typename Arc::Weight Weight;

  Matcher(const F &fst, MatchType match_type) {
    fst.InitMatcher(match_type, &data_);
    if (!data_.base)
      data_.base = new SortedMatcher<F>(fst, match_type);
  }

  Matcher(const Matcher<F> &matcher) {
    data_.base = matcher.data_.base->Copy();
  }

  ~Matcher() {
    delete data_.base;
  }

  MatchType Type(bool test) const {
    return data_.base->Type(test);
  }

  void SetState(StateId s) {
    data_.base->SetState(s);
  }

  bool Find(Label label) {
    return data_.base->Find(label);
  }

  bool Done() const {
    return data_.base->Done();
  }

  const Arc& Value() const {
    return data_.base->Value();
  }

  void Next() {
    data_.base->Next();
  }

  uint64 Properties(uint64 props) const {
    return data_.base->Properties(props);
  }

 private:
  MatcherData<Arc> data_;

  void operator=(const Matcher<Arc> &);  // disallow
};

}  // namespace fst



#endif  // FST_LIB_MATCHER_H__
