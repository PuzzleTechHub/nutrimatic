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
// Copyright 2005-2010 Google, Inc.
// Author: riley@google.com (Michael Riley)
// Author: rws@google.com (Richard Sproat)
//
// \file
// Compose an MPDT and an FST.

#ifndef FST_EXTENSIONS_MPDT_COMPOSE_H__
#define FST_EXTENSIONS_MPDT_COMPOSE_H__

#include <list>

#include <fst/extensions/pdt/compose.h>
#include <fst/extensions/mpdt/mpdt.h>
#include <fst/compose.h>

namespace fst {

template <class F>
class MPdtParenFilter {
 public:
  typedef typename F::FST1 FST1;
  typedef typename F::FST2 FST2;
  typedef typename F::Arc Arc;
  typedef typename Arc::StateId StateId;
  typedef typename Arc::Label Label;
  typedef typename Arc::Weight Weight;
  typedef typename F::Matcher1 Matcher1;
  typedef typename F::Matcher2 Matcher2;
  typedef typename F::FilterState FilterState1;
  typedef StateId StackId;
  typedef MPdtStack<StackId, Label> ParenStack;
  typedef IntegerFilterState<StackId> FilterState2;
  typedef PairFilterState<FilterState1, FilterState2> FilterState;
  typedef MPdtParenFilter<F> Filter;

  MPdtParenFilter(const FST1 &fst1, const FST2 &fst2,
                  Matcher1 *matcher1 = 0,  Matcher2 *matcher2 = 0,
                  const vector<pair<Label, Label> > *parens = 0,
                  const vector<typename Arc::Label> *assignments = 0,
                  bool expand = false, bool keep_parens = true)
      : filter_(fst1, fst2, matcher1, matcher2),
        parens_(parens ? *parens : vector<pair<Label, Label> >()),
        assignments_(assignments ? *assignments : vector<int>()),
        expand_(expand),
        keep_parens_(keep_parens),
        f_(FilterState::NoState()),
        stack_(parens_, assignments_),
        paren_id_(-1) {
    if (parens) {
      for (size_t i = 0; i < parens->size(); ++i) {
        const pair<Label, Label>  &p = (*parens)[i];
        parens_.push_back(p);
        GetMatcher1()->AddOpenParen(p.first);
        GetMatcher2()->AddOpenParen(p.first);
        if (!expand_) {
          GetMatcher1()->AddCloseParen(p.second);
          GetMatcher2()->AddCloseParen(p.second);
        }
      }
    }
  }

  MPdtParenFilter(const Filter &filter, bool safe = false)
      : filter_(filter.filter_, safe),
        parens_(filter.parens_),
        expand_(filter.expand_),
        keep_parens_(filter.keep_parens_),
        f_(FilterState::NoState()),
        stack_(filter.parens_, filter.assignments_),
        paren_id_(-1) { }

  FilterState Start() const {
    return FilterState(filter_.Start(), FilterState2(0));
  }

  void SetState(StateId s1, StateId s2, const FilterState &f) {
    f_ = f;
    filter_.SetState(s1, s2, f_.GetState1());
    if (!expand_)
      return;

    ssize_t paren_id = stack_.Top(f.GetState2().GetState());
    if (paren_id != paren_id_) {
      if (paren_id_ != -1) {
        GetMatcher1()->RemoveCloseParen(parens_[paren_id_].second);
        GetMatcher2()->RemoveCloseParen(parens_[paren_id_].second);
      }
      paren_id_ = paren_id;
      if (paren_id_ != -1) {
        GetMatcher1()->AddCloseParen(parens_[paren_id_].second);
        GetMatcher2()->AddCloseParen(parens_[paren_id_].second);
      }
    }
  }

  FilterState FilterArc(Arc *arc1, Arc *arc2) const {
    FilterState1 f1 = filter_.FilterArc(arc1, arc2);
    const FilterState2 &f2 = f_.GetState2();
    if (f1 == FilterState1::NoState())
      return FilterState::NoState();

    if (arc1->olabel == kNoLabel && arc2->ilabel) {         // arc2 parentheses
      if (keep_parens_) {
        arc1->ilabel = arc2->ilabel;
      } else if (arc2->ilabel) {
        arc2->olabel = arc1->ilabel;
      }
      return FilterParen(arc2->ilabel, f1, f2);
    } else if (arc2->ilabel == kNoLabel && arc1->olabel) {  // arc1 parentheses
      if (keep_parens_) {
        arc2->olabel = arc1->olabel;
      } else {
        arc1->ilabel = arc2->olabel;
      }
      return FilterParen(arc1->olabel, f1, f2);
    } else {
      return FilterState(f1, f2);
    }
  }

  void FilterFinal(Weight *w1, Weight *w2) const {
    if (f_.GetState2().GetState() != 0)
      *w1 = Weight::Zero();
    filter_.FilterFinal(w1, w2);
  }

  // Return resp matchers. Ownership stays with filter.
  Matcher1 *GetMatcher1() { return filter_.GetMatcher1(); }
  Matcher2 *GetMatcher2() { return filter_.GetMatcher2(); }

  uint64 Properties(uint64 iprops) const {
    uint64 oprops = filter_.Properties(iprops);
    return oprops & kILabelInvariantProperties & kOLabelInvariantProperties;
  }

 private:
  const FilterState FilterParen(Label label, const FilterState1 &f1,
                                const FilterState2 &f2) const {
    if (!expand_)
      return FilterState(f1, f2);

    StackId stack_id = stack_.Find(f2.GetState(), label);
    if (stack_id < 0) {
      return FilterState::NoState();
    } else {
      return FilterState(f1, FilterState2(stack_id));
    }
  }

  F filter_;
  vector<pair<Label, Label> > parens_;
  vector<typename Arc::Label> assignments_;
  bool expand_;                    // Expands to FST
  bool keep_parens_;               // Retains parentheses in output
  FilterState f_;                  // Current filter state
  mutable ParenStack stack_;
  ssize_t paren_id_;
};

// Class to setup composition options for PDT composition.
// Default is for the PDT as the first composition argument.
template <class Arc, bool left_pdt = true>
class MPdtComposeFstOptions : public
ComposeFstOptions<Arc,
                  ParenMatcher< Fst<Arc> >,
                  MPdtParenFilter<AltSequenceComposeFilter<
                                    ParenMatcher< Fst<Arc> > > > > {
 public:
  typedef typename Arc::Label Label;
  typedef ParenMatcher< Fst<Arc> > MPdtMatcher;
  typedef MPdtParenFilter<AltSequenceComposeFilter<MPdtMatcher> > MPdtFilter;
  typedef ComposeFstOptions<Arc, MPdtMatcher, MPdtFilter> COptions;
  using COptions::matcher1;
  using COptions::matcher2;
  using COptions::filter;

  MPdtComposeFstOptions(const Fst<Arc> &ifst1,
                        const vector<pair<Label, Label> > &parens,
                        const vector<typename Arc::Label> &assignments,
                        const Fst<Arc> &ifst2, bool expand = false,
                        bool keep_parens = true) {
    matcher1 = new MPdtMatcher(ifst1, MATCH_OUTPUT, kParenList);
    matcher2 = new MPdtMatcher(ifst2, MATCH_INPUT, kParenLoop);

    filter = new MPdtFilter(ifst1, ifst2, matcher1, matcher2, &parens,
                           &assignments, expand, keep_parens);
  }
};

// Class to setup composition options for PDT with FST composition.
// Specialization is for the FST as the first composition argument.
template <class Arc>
class MPdtComposeFstOptions<Arc, false> : public
ComposeFstOptions<Arc,
                  ParenMatcher< Fst<Arc> >,
                  MPdtParenFilter<SequenceComposeFilter<
                                    ParenMatcher< Fst<Arc> > > > > {
 public:
  typedef typename Arc::Label Label;
  typedef ParenMatcher< Fst<Arc> > MPdtMatcher;
  typedef MPdtParenFilter<SequenceComposeFilter<MPdtMatcher> > MPdtFilter;
  typedef ComposeFstOptions<Arc, MPdtMatcher, MPdtFilter> COptions;
  using COptions::matcher1;
  using COptions::matcher2;
  using COptions::filter;

  MPdtComposeFstOptions(const Fst<Arc> &ifst1,
                        const Fst<Arc> &ifst2,
                        const vector<pair<Label, Label> > &parens,
                        const vector<typename Arc::Label> &assignments,
                        bool expand = false, bool keep_parens = true) {
    matcher1 = new MPdtMatcher(ifst1, MATCH_OUTPUT, kParenLoop);
    matcher2 = new MPdtMatcher(ifst2, MATCH_INPUT, kParenList);

    filter = new MPdtFilter(ifst1, ifst2, matcher1, matcher2, &parens,
                            &assignments, expand, keep_parens);
  }
};

struct MPdtComposeOptions {
  bool connect;  // Connect output
  PdtComposeFilter filter_type;  // Which pre-defined filter to use

  explicit MPdtComposeOptions(bool c, PdtComposeFilter ft = PAREN_FILTER)
      : connect(c), filter_type(ft) {}
  MPdtComposeOptions() : connect(true), filter_type(PAREN_FILTER) {}
};

// Composes pushdown transducer (PDT) encoded as an FST (1st arg) and
// an FST (2nd arg) with the result also a PDT encoded as an Fst. (3rd arg).
// In the PDTs, some transitions are labeled with open or close
// parentheses. To be interpreted as a PDT, the parens must balance on
// a path (see MPdtExpand()). The open-close parenthesis label pairs
// are passed in 'parens'.
template <class Arc>
void Compose(const Fst<Arc> &ifst1,
             const vector<pair<typename Arc::Label,
                               typename Arc::Label> > &parens,
             const vector<typename Arc::Label> &assignments,
             const Fst<Arc> &ifst2,
             MutableFst<Arc> *ofst,
             const MPdtComposeOptions &opts = MPdtComposeOptions()) {
  bool expand = opts.filter_type != PAREN_FILTER;
  bool keep_parens = opts.filter_type != EXPAND_FILTER;
  MPdtComposeFstOptions<Arc, true> copts(ifst1, parens, assignments, ifst2,
                                         expand, keep_parens);
  copts.gc_limit = 0;
  *ofst = ComposeFst<Arc>(ifst1, ifst2, copts);
  if (opts.connect)
    Connect(ofst);
}

// Composes an FST (1st arg) and pushdown transducer (PDT) encoded as
// an FST (2nd arg) with the result also a PDT encoded as an Fst (3rd arg).
// In the PDTs, some transitions are labeled with open or close
// parentheses. To be interpreted as a PDT, the parens must balance on
// a path (see ExpandFst()). The open-close parenthesis label pairs
// are passed in 'parens'.
template <class Arc>
void Compose(const Fst<Arc> &ifst1,
             const Fst<Arc> &ifst2,
             const vector<pair<typename Arc::Label,
                               typename Arc::Label> > &parens,
             const vector<typename Arc::Label> &assignments,
             MutableFst<Arc> *ofst,
             const MPdtComposeOptions &opts = MPdtComposeOptions()) {
  bool expand = opts.filter_type != PAREN_FILTER;
  bool keep_parens = opts.filter_type != EXPAND_FILTER;
  MPdtComposeFstOptions<Arc, false> copts(ifst1, ifst2, parens,
                                          assignments, expand, keep_parens);
  copts.gc_limit = 0;
  *ofst = ComposeFst<Arc>(ifst1, ifst2, copts);
  if (opts.connect)
    Connect(ofst);
}

}  // namespace fst

#endif  // FST_EXTENSIONS_MPDT_COMPOSE_H__
