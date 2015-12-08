// expand.h

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
// Expand an MPDT to an FST.

#ifndef FST_EXTENSIONS_MPDT_EXPAND_H__
#define FST_EXTENSIONS_MPDT_EXPAND_H__

#include <vector>
using std::vector;

#include <fst/extensions/mpdt/mpdt.h>
#include <fst/extensions/pdt/paren.h>
#include <fst/cache.h>
#include <fst/mutable-fst.h>
#include <fst/queue.h>
#include <fst/state-table.h>
#include <fst/test-properties.h>

namespace fst {

template <class Arc>
struct ExpandFstOptions : public CacheOptions {
  bool keep_parentheses;
  MPdtStack<typename Arc::StateId, typename Arc::Label> *stack;
  PdtStateTable<typename Arc::StateId, typename Arc::StateId> *state_table;

  ExpandFstOptions(
      const CacheOptions &opts = CacheOptions(),
      bool kp = false,
      MPdtStack<typename Arc::StateId, typename Arc::Label> *s = 0,
      PdtStateTable<typename Arc::StateId, typename Arc::StateId> *st = 0)
      : CacheOptions(opts), keep_parentheses(kp), stack(s), state_table(st) {}
};

// Properties for an expanded PDT.
inline uint64 ExpandProperties(uint64 inprops) {
  return inprops & (kAcceptor | kAcyclic | kInitialAcyclic | kUnweighted);
}

// Implementation class for ExpandFst
template <class A>
class ExpandFstImpl
    : public CacheImpl<A> {
 public:
  using FstImpl<A>::SetType;
  using FstImpl<A>::SetProperties;
  using FstImpl<A>::Properties;
  using FstImpl<A>::SetInputSymbols;
  using FstImpl<A>::SetOutputSymbols;

  using CacheBaseImpl< CacheState<A> >::PushArc;
  using CacheBaseImpl< CacheState<A> >::HasArcs;
  using CacheBaseImpl< CacheState<A> >::HasFinal;
  using CacheBaseImpl< CacheState<A> >::HasStart;
  using CacheBaseImpl< CacheState<A> >::SetArcs;
  using CacheBaseImpl< CacheState<A> >::SetFinal;
  using CacheBaseImpl< CacheState<A> >::SetStart;

  typedef A Arc;
  typedef typename A::Label Label;
  typedef typename A::Weight Weight;
  typedef typename A::StateId StateId;
  typedef StateId StackId;
  typedef PdtStateTuple<StateId, StackId> StateTuple;

  ExpandFstImpl(const Fst<A> &fst,
                const vector<pair<typename Arc::Label,
                                  typename Arc::Label> > &parens,
                const vector<typename Arc::Label> &assignments,
                const ExpandFstOptions<A> &opts)
      : CacheImpl<A>(opts), fst_(fst.Copy()),
        stack_(opts.stack ?
               opts.stack:
               new MPdtStack<StateId, Label>(parens, assignments)),
        state_table_(opts.state_table ? opts.state_table :
                     new PdtStateTable<StateId, StackId>()),
        own_stack_(opts.stack == 0), own_state_table_(opts.state_table == 0),
        keep_parentheses_(opts.keep_parentheses) {
    SetType("expand");

    uint64 props = fst.Properties(kFstProperties, false);
    SetProperties(ExpandProperties(props), kCopyProperties);

    SetInputSymbols(fst.InputSymbols());
    SetOutputSymbols(fst.OutputSymbols());
  }

  ExpandFstImpl(const ExpandFstImpl &impl)
      : CacheImpl<A>(impl),
        fst_(impl.fst_->Copy(true)),
        stack_(new MPdtStack<StateId, Label>(*impl.stack_)),
        state_table_(new PdtStateTable<StateId, StackId>()),
        own_stack_(true), own_state_table_(true),
        keep_parentheses_(impl.keep_parentheses_) {
    SetType("expand");
    SetProperties(impl.Properties(), kCopyProperties);
    SetInputSymbols(impl.InputSymbols());
    SetOutputSymbols(impl.OutputSymbols());
  }

  ~ExpandFstImpl() {
    delete fst_;
    if (own_stack_)
      delete stack_;
    if (own_state_table_)
      delete state_table_;
  }

  StateId Start() {
    if (!HasStart()) {
      StateId s = fst_->Start();
      if (s == kNoStateId)
        return kNoStateId;
      StateTuple tuple(s, 0);
      StateId start = state_table_->FindState(tuple);
      SetStart(start);
    }
    return CacheImpl<A>::Start();
  }

  Weight Final(StateId s) {
    if (!HasFinal(s)) {
      const StateTuple &tuple = state_table_->Tuple(s);
      Weight w = fst_->Final(tuple.state_id);
      if (w != Weight::Zero() && tuple.stack_id == 0)
        SetFinal(s, w);
      else
        SetFinal(s, Weight::Zero());
    }
    return CacheImpl<A>::Final(s);
  }

  size_t NumArcs(StateId s) {
    if (!HasArcs(s)) {
      ExpandState(s);
    }
    return CacheImpl<A>::NumArcs(s);
  }

  size_t NumInputEpsilons(StateId s) {
    if (!HasArcs(s))
      ExpandState(s);
    return CacheImpl<A>::NumInputEpsilons(s);
  }

  size_t NumOutputEpsilons(StateId s) {
    if (!HasArcs(s))
      ExpandState(s);
    return CacheImpl<A>::NumOutputEpsilons(s);
  }

  void InitArcIterator(StateId s, ArcIteratorData<A> *data) {
    if (!HasArcs(s))
      ExpandState(s);
    CacheImpl<A>::InitArcIterator(s, data);
  }

  // Computes the outgoing transitions from a state, creating new destination
  // states as needed.
  void ExpandState(StateId s) {
    StateTuple tuple = state_table_->Tuple(s);
    for (ArcIterator< Fst<A> > aiter(*fst_, tuple.state_id);
         !aiter.Done(); aiter.Next()) {
      Arc arc = aiter.Value();
      StackId stack_id = stack_->Find(tuple.stack_id, arc.ilabel);
      if (stack_id == -1) {
        // Non-matching close parenthesis
        continue;
      } else if ((stack_id != tuple.stack_id) && !keep_parentheses_) {
        // Stack push/pop
        arc.ilabel = arc.olabel = 0;
      }

      StateTuple ntuple(arc.nextstate, stack_id);
      arc.nextstate = state_table_->FindState(ntuple);
      PushArc(s, arc);
    }
    SetArcs(s);
  }

  const MPdtStack<StackId, Label> &GetStack() const { return *stack_; }

  const PdtStateTable<StateId, StackId> &GetStateTable() const {
    return *state_table_;
  }

 private:
  const Fst<A> *fst_;

  MPdtStack<StackId, Label> *stack_;
  PdtStateTable<StateId, StackId> *state_table_;
  bool own_stack_;
  bool own_state_table_;
  bool keep_parentheses_;

  void operator=(const ExpandFstImpl<A> &);  // disallow
};

// Expands a multi-pushdown transducer (MPDT) encoded as an FST into an FST.
// This version is a delayed Fst. In the MPDT, some transitions are labeled with
// open or close parentheses. To be interpreted as an MPDT, the parens for each
// stack must balance on a path. The open-close parenthesis label pair sets are
// passed in 'parens', and the assignment of those pairs to stacks in
// 'paren_assignments'. The expansion enforces the parenthesis constraints. The
// MPDT must be expandable as an FST.
//
// This class attaches interface to implementation and handles
// reference counting, delegating most methods to ImplToFst.
template <class A>
class ExpandFst : public ImplToFst< ExpandFstImpl<A> > {
 public:
  friend class ArcIterator< ExpandFst<A> >;
  friend class StateIterator< ExpandFst<A> >;

  typedef A Arc;
  typedef typename A::Label Label;
  typedef typename A::Weight Weight;
  typedef typename A::StateId StateId;
  typedef StateId StackId;
  typedef DefaultCacheStore<A> Store;
  typedef typename Store::State State;
  typedef ExpandFstImpl<A> Impl;

  ExpandFst(const Fst<A> &fst,
            const vector<pair<typename Arc::Label,
                              typename Arc::Label> > &parens,
            const vector<typename Arc::Label> &assignments)
      : ImplToFst<Impl>(new Impl(fst, parens, assignments,
                                 ExpandFstOptions<A>())) {}

  ExpandFst(const Fst<A> &fst,
            const vector<pair<typename Arc::Label,
                              typename Arc::Label> > &parens,
            const vector<typename Arc::Label> &assignments,
            const ExpandFstOptions<A> &opts)
      : ImplToFst<Impl>(new Impl(fst, parens, assignments, opts)) {}

  // See Fst<>::Copy() for doc.
  ExpandFst(const ExpandFst<A> &fst, bool safe = false)
      : ImplToFst<Impl>(fst, safe) {}

  // Get a copy of this ExpandFst. See Fst<>::Copy() for further doc.
  virtual ExpandFst<A> *Copy(bool safe = false) const {
    return new ExpandFst<A>(*this, safe);
  }

  virtual inline void InitStateIterator(StateIteratorData<A> *data) const;

  virtual void InitArcIterator(StateId s, ArcIteratorData<A> *data) const {
    GetImpl()->InitArcIterator(s, data);
  }

  const MPdtStack<StackId, Label> &GetStack() const {
    return GetImpl()->GetStack();
  }

  const PdtStateTable<StateId, StackId> &GetStateTable() const {
    return GetImpl()->GetStateTable();
  }

 private:
  // Makes visible to friends.
  Impl *GetImpl() const { return ImplToFst<Impl>::GetImpl(); }

  void operator=(const ExpandFst<A> &fst);  // Disallow
};


// Specialization for ExpandFst.
template<class A>
class StateIterator< ExpandFst<A> >
    : public CacheStateIterator< ExpandFst<A> > {
 public:
  explicit StateIterator(const ExpandFst<A> &fst)
      : CacheStateIterator< ExpandFst<A> >(fst, fst.GetImpl()) {}
};


// Specialization for ExpandFst.
template <class A>
class ArcIterator< ExpandFst<A> >
    : public CacheArcIterator< ExpandFst<A> > {
 public:
  typedef typename A::StateId StateId;

  ArcIterator(const ExpandFst<A> &fst, StateId s)
      : CacheArcIterator< ExpandFst<A> >(fst.GetImpl(), s) {
    if (!fst.GetImpl()->HasArcs(s))
      fst.GetImpl()->ExpandState(s);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ArcIterator);
};


template <class A> inline
void ExpandFst<A>::InitStateIterator(StateIteratorData<A> *data) const {
  data->base = new StateIterator< ExpandFst<A> >(*this);
}

//
// Expand() Functions
//

template <class Arc>
struct ExpandOptions {
  bool connect;
  bool keep_parentheses;

  ExpandOptions(bool c  = true, bool k = false)
      : connect(c), keep_parentheses(k) {}
};

// Expands a multi-pushdown transducer (MPDT) encoded as an FST into an FST.
// This version writes the expanded PDT result to a MutableFst.  In the MPDT,
// some transitions are labeled with open or close parentheses. To be
// interpreted as an MPDT, the parens for each stack must balance on a path. The
// open-close parenthesis label pair sets are passed in 'parens', and the
// assignment of those pairs to stacks in 'paren_assignments'. The expansion
// enforces the parenthesis constraints. The MPDT must be expandable as an FST.
template <class Arc>
void Expand(
    const Fst<Arc> &ifst,
    const vector<pair<typename Arc::Label, typename Arc::Label> > &parens,
    const vector<typename Arc::Label> &assignments,
    MutableFst<Arc> *ofst,
    const ExpandOptions<Arc> &opts) {
  typedef typename Arc::Label Label;
  typedef typename Arc::StateId StateId;
  typedef typename Arc::Weight Weight;
  typedef typename ExpandFst<Arc>::StackId StackId;

  ExpandFstOptions<Arc> eopts;
  eopts.gc_limit = 0;
  eopts.keep_parentheses = opts.keep_parentheses;
  *ofst = ExpandFst<Arc>(ifst, parens, assignments, eopts);

  if (opts.connect)
    Connect(ofst);
}

// Expands a multi-pushdown transducer (MPDT) encoded as an FST into an FST.
// This version writes the expanded PDT result to a MutableFst.  In the MPDT,
// some transitions are labeled with open or close parentheses. To be
// interpreted as an MPDT, the parens for each stack must balance on a path. The
// open-close parenthesis label pair sets are passed in 'parens', and the
// assignment of those pairs to stacks in 'paren_assignments'. The expansion
// enforces the parenthesis constraints. The MPDT must be expandable as an FST.
template<class Arc>
void Expand(
    const Fst<Arc> &ifst,
    const vector<pair<typename Arc::Label, typename Arc::Label> > &parens,
    const vector<typename Arc::Label> &assignments,
    MutableFst<Arc> *ofst,
    bool connect = true, bool keep_parentheses = false) {
  Expand(ifst, parens, assignments, ofst,
         ExpandOptions<Arc>(connect, keep_parentheses));
}

}  // namespace fst

#endif  // FST_EXTENSIONS_MPDT_EXPAND_H__
