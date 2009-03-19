// rmepsilon.h

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
// Functions and classes that implemement epsilon-removal.

#ifndef FST_LIB_RMEPSILON_H__
#define FST_LIB_RMEPSILON_H__

#include <tr1/unordered_map>
using std::tr1::unordered_map;
#include <fst/slist.h>
#include <stack>
#include <string>
#include <utility>
#include <vector>

#include <fst/arcfilter.h>
#include <fst/cache.h>
#include <fst/connect.h>
#include <fst/factor-weight.h>
#include <fst/invert.h>
#include <fst/map.h>
#include <fst/prune.h>
#include <fst/queue.h>
#include <fst/shortest-distance.h>
#include <fst/topsort.h>

namespace fst {

template <class Arc, class Queue>
class RmEpsilonOptions
    : public ShortestDistanceOptions<Arc, Queue, EpsilonArcFilter<Arc> > {
 public:
  typedef typename Arc::StateId StateId;
  typedef typename Arc::Weight Weight;

  bool connect;              // Connect output
  Weight weight_threshold;   // Pruning weight threshold.
  StateId state_threshold;   // Pruning state threshold.

  explicit RmEpsilonOptions(Queue *q, float d = kDelta, bool c = true,
                            Weight w = Weight::Zero(),
                            StateId n = kNoStateId)
      : ShortestDistanceOptions< Arc, Queue, EpsilonArcFilter<Arc> >(
          q, EpsilonArcFilter<Arc>(), kNoStateId, d),
        connect(c), weight_threshold(w), state_threshold(n) {}
 private:
  RmEpsilonOptions();  // disallow
};

// Computation state of the epsilon-removal algorithm.
template <class Arc, class Queue>
class RmEpsilonState {
 public:
  typedef typename Arc::Label Label;
  typedef typename Arc::StateId StateId;
  typedef typename Arc::Weight Weight;

  RmEpsilonState(const Fst<Arc> &fst,
                 vector<Weight> *distance,
                 const RmEpsilonOptions<Arc, Queue> &opts)
      : fst_(fst), distance_(distance), sd_state_(fst_, distance, opts, true),
        expand_id_(0) {}

  // Compute arcs and final weight for state 's'
  void Expand(StateId s);

  // Returns arcs of expanded state.
  vector<Arc> &Arcs() { return arcs_; }

  // Returns final weight of expanded state.
  const Weight &Final() const { return final_; }

 private:
  struct Element {
    Label ilabel;
    Label olabel;
    StateId nextstate;

    Element() {}

    Element(Label i, Label o, StateId s)
        : ilabel(i), olabel(o), nextstate(s) {}
  };

  class ElementKey {
   public:
    size_t operator()(const Element& e) const {
      return static_cast<size_t>(e.nextstate);
      return static_cast<size_t>(e.nextstate +
                                 e.ilabel * kPrime0 +
                                 e.olabel * kPrime1);
    }

   private:
    static const int kPrime0 = 7853;
    static const int kPrime1 = 7867;
  };

  class ElementEqual {
   public:
    bool operator()(const Element &e1, const Element &e2) const {
      return (e1.ilabel == e2.ilabel) &&  (e1.olabel == e2.olabel)
                         && (e1.nextstate == e2.nextstate);
    }
  };

 private:
  typedef unordered_map<Element, pair<StateId, size_t>,
                   ElementKey, ElementEqual> ElementMap;

  const Fst<Arc> &fst_;
  // Distance from state being expanded in epsilon-closure.
  vector<Weight> *distance_;
  // Shortest distance algorithm computation state.
  ShortestDistanceState<Arc, Queue, EpsilonArcFilter<Arc> > sd_state_;
  // Maps an element 'e' to a pair 'p' corresponding to a position
  // in the arcs vector of the state being expanded. 'e' corresponds
  // to the position 'p.second' in the 'arcs_' vector if 'p.first' is
  // equal to the state being expanded.
  ElementMap element_map_;
  EpsilonArcFilter<Arc> eps_filter_;
  stack<StateId> eps_queue_;      // Queue used to visit the epsilon-closure
  vector<bool> visited_;          // '[i] = true' if state 'i' has been visited
  slist<StateId> visited_states_; // List of visited states
  vector<Arc> arcs_;              // Arcs of state being expanded
  Weight final_;                  // Final weight of state being expanded
  StateId expand_id_;             // Unique ID characterizing each call to Expand

  DISALLOW_COPY_AND_ASSIGN(RmEpsilonState);
};


template <class Arc, class Queue>
void RmEpsilonState<Arc,Queue>::Expand(typename Arc::StateId source) {
   sd_state_.ShortestDistance(source);
   eps_queue_.push(source);
   final_ = Weight::Zero();
   arcs_.clear();

   while (!eps_queue_.empty()) {
     StateId state = eps_queue_.top();
     eps_queue_.pop();

     while (visited_.size() <= state) visited_.push_back(false);
     if (visited_[state]) continue;
     visited_[state] = true;
     visited_states_.push_front(state);

     for (ArcIterator< Fst<Arc> > ait(fst_, state);
          !ait.Done();
          ait.Next()) {
       Arc arc = ait.Value();
       arc.weight = Times((*distance_)[state], arc.weight);

       if (eps_filter_(arc)) {
         while (visited_.size() <= arc.nextstate)
           visited_.push_back(false);
         if (!visited_[arc.nextstate])
           eps_queue_.push(arc.nextstate);
       } else {
          Element element(arc.ilabel, arc.olabel, arc.nextstate);
          typename ElementMap::iterator it = element_map_.find(element);
          if (it == element_map_.end()) {
            element_map_.insert(
                pair<Element, pair<StateId, size_t> >
                (element, pair<StateId, size_t>(expand_id_, arcs_.size())));
            arcs_.push_back(arc);
          } else {
            if (((*it).second).first == expand_id_) {
              Weight &w = arcs_[((*it).second).second].weight;
              w = Plus(w, arc.weight);
            } else {
              ((*it).second).first = expand_id_;
              ((*it).second).second = arcs_.size();
              arcs_.push_back(arc);
            }
          }
        }
     }
     final_ = Plus(final_, Times((*distance_)[state], fst_.Final(state)));
   }

   while (!visited_states_.empty()) {
     visited_[visited_states_.front()] = false;
     visited_states_.pop_front();
   }
   ++expand_id_;
}


// Removes epsilon-transitions (when both the input and output label
// are an epsilon) from a transducer. The result will be an equivalent
// FST that has no such epsilon transitions.  This version modifies
// its input. It allows fine control via the options argument; see
// below for a simpler interface.
//
// The vector 'distance' will be used to hold the shortest distances
// during the epsilon-closure computation. The state queue discipline
// and convergence delta are taken in the options argument.
template <class Arc, class Queue>
void RmEpsilon(MutableFst<Arc> *fst,
               vector<typename Arc::Weight> *distance,
               const RmEpsilonOptions<Arc, Queue> &opts) {
  typedef typename Arc::StateId StateId;
  typedef typename Arc::Weight Weight;
  typedef typename Arc::Label Label;

  // States sorted in topological order when (acyclic) or generic
  // topological order (cyclic).
  vector<StateId> states;

  if (fst->Properties(kTopSorted, false) & kTopSorted) {
    for (StateId i = 0; i < fst->NumStates(); i++)
      states.push_back(i);
  } else if (fst->Properties(kAcyclic, false) & kAcyclic) {
    vector<StateId> order;
    bool acyclic;
    TopOrderVisitor<Arc> top_order_visitor(&order, &acyclic);
    DfsVisit(*fst, &top_order_visitor, EpsilonArcFilter<Arc>());
    if (!acyclic)
      LOG(FATAL) << "RmEpsilon: not acyclic though property bit is set";
    states.resize(order.size());
    for (StateId i = 0; i < order.size(); i++)
      states[order[i]] = i;
  } else {
     uint64 props;
     vector<StateId> scc;
     SccVisitor<Arc> scc_visitor(&scc, 0, 0, &props);
     DfsVisit(*fst, &scc_visitor, EpsilonArcFilter<Arc>());
     vector<StateId> first(scc.size(), kNoStateId);
     vector<StateId> next(scc.size(), kNoStateId);
     for (StateId i = 0; i < scc.size(); i++) {
       if (first[scc[i]] != kNoStateId)
         next[i] = first[scc[i]];
       first[scc[i]] = i;
     }
     for (StateId i = 0; i < first.size(); i++)
       for (StateId j = first[i]; j != kNoStateId; j = next[j])
         states.push_back(j);
  }

  RmEpsilonState<Arc, Queue>
    rmeps_state(*fst, distance, opts);

  while (!states.empty()) {
    StateId state = states.back();
    states.pop_back();
    rmeps_state.Expand(state);
    fst->SetFinal(state, rmeps_state.Final());
    fst->DeleteArcs(state);
    vector<Arc> &arcs = rmeps_state.Arcs();
    while (!arcs.empty()) {
      fst->AddArc(state, arcs.back());
      arcs.pop_back();
    }
  }

  fst->SetProperties(RmEpsilonProperties(
                         fst->Properties(kFstProperties, false)),
                     kFstProperties);

  if (opts.weight_threshold != Weight::Zero() ||
      opts.state_threshold != kNoStateId)
    Prune(fst, opts.weight_threshold, opts.state_threshold);
  if (opts.connect && (opts.weight_threshold == Weight::Zero() ||
                       opts.state_threshold != kNoStateId))
    Connect(fst);
}


// Removes epsilon-transitions (when both the input and output label
// are an epsilon) from a transducer. The result will be an equivalent
// FST that has no such epsilon transitions. This version modifies its
// input. It has a simplified interface; see above for a version that
// allows finer control.
//
// Complexity:
// - Time:
//   - Unweighted: O(V2 + V E)
//   - Acyclic: O(V2 + V E)
//   - Tropical semiring: O(V2 log V + V E)
//   - General: exponential
// - Space: O(V E)
// where V = # of states visited, E = # of arcs.
//
// References:
// - Mehryar Mohri. Generic Epsilon-Removal and Input
//   Epsilon-Normalization Algorithms for Weighted Transducers,
//   "International Journal of Computer Science", 13(1):129-143 (2002).
template <class Arc>
void RmEpsilon(MutableFst<Arc> *fst,
               bool connect = true,
               typename Arc::Weight weight_threshold = Arc::Weight::Zero(),
               typename Arc::StateId state_threshold = kNoStateId,
               float delta = kDelta) {
  typedef typename Arc::StateId StateId;
  typedef typename Arc::Weight Weight;
  typedef typename Arc::Label Label;

  vector<Weight> distance;
  AutoQueue<StateId> state_queue(*fst, &distance, EpsilonArcFilter<Arc>());
  RmEpsilonOptions<Arc, AutoQueue<StateId> >
      opts(&state_queue, delta, connect, weight_threshold, state_threshold);

  RmEpsilon(fst, &distance, opts);
}


struct RmEpsilonFstOptions : CacheOptions {
  float delta;

  RmEpsilonFstOptions(const CacheOptions &opts, float delta = kDelta)
      : CacheOptions(opts), delta(delta) {}

  explicit RmEpsilonFstOptions(float delta = kDelta) : delta(delta) {}
};


// Implementation of delayed RmEpsilonFst.
template <class A>
class RmEpsilonFstImpl : public CacheImpl<A> {
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

  RmEpsilonFstImpl(const Fst<A>& fst, const RmEpsilonFstOptions &opts)
      : CacheImpl<A>(opts),
        fst_(fst.Copy()),
        delta_(opts.delta),
        rmeps_state_(
            *fst_,
            &distance_,
            RmEpsilonOptions<A, FifoQueue<StateId> >(&queue_, delta_, false)) {
    SetType("rmepsilon");
    uint64 props = fst.Properties(kFstProperties, false);
    SetProperties(RmEpsilonProperties(props, true), kCopyProperties);
    SetInputSymbols(fst.InputSymbols());
    SetOutputSymbols(fst.OutputSymbols());
  }

  RmEpsilonFstImpl(const RmEpsilonFstImpl &impl)
      : CacheImpl<A>(impl),
        fst_(impl.fst_->Copy(true)),
        delta_(impl.delta_),
        rmeps_state_(
            *fst_,
            &distance_,
            RmEpsilonOptions<A, FifoQueue<StateId> >(&queue_, delta_, false)) {
    SetType("rmepsilon");
    SetProperties(impl.Properties(), kCopyProperties);
    SetInputSymbols(impl.InputSymbols());
    SetOutputSymbols(impl.OutputSymbols());
  }

  ~RmEpsilonFstImpl() {
    delete fst_;
  }

  StateId Start() {
    if (!HasStart()) {
      SetStart(fst_->Start());
    }
    return CacheImpl<A>::Start();
  }

  Weight Final(StateId s) {
    if (!HasFinal(s)) {
      Expand(s);
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

  void Expand(StateId s) {
    rmeps_state_.Expand(s);
    SetFinal(s, rmeps_state_.Final());
    vector<A> &arcs = rmeps_state_.Arcs();
    while (!arcs.empty()) {
      AddArc(s, arcs.back());
      arcs.pop_back();
    }
    SetArcs(s);
  }

 private:
  const Fst<A> *fst_;
  float delta_;
  vector<Weight> distance_;
  FifoQueue<StateId> queue_;
  RmEpsilonState<A, FifoQueue<StateId> > rmeps_state_;

  void operator=(const RmEpsilonFstImpl<A> &);  // disallow
};


// Removes epsilon-transitions (when both the input and output label
// are an epsilon) from a transducer. The result will be an equivalent
// FST that has no such epsilon transitions.  This version is a
// delayed Fst.
//
// Complexity:
// - Time:
//   - Unweighted: O(v^2 + v e)
//   - General: exponential
// - Space: O(v e)
// where v = # of states visited, e = # of arcs visited. Constant time
// to visit an input state or arc is assumed and exclusive of caching.
//
// References:
// - Mehryar Mohri. Generic Epsilon-Removal and Input
//   Epsilon-Normalization Algorithms for Weighted Transducers,
//   "International Journal of Computer Science", 13(1):129-143 (2002).
template <class A>
class RmEpsilonFst : public Fst<A> {
 public:
  friend class ArcIterator< RmEpsilonFst<A> >;
  friend class CacheStateIterator< RmEpsilonFst<A> >;
  friend class CacheArcIterator< RmEpsilonFst<A> >;

  typedef A Arc;
  typedef typename A::Weight Weight;
  typedef typename A::StateId StateId;
  typedef CacheState<A> State;

  RmEpsilonFst(const Fst<A> &fst)
      : impl_(new RmEpsilonFstImpl<A>(fst, RmEpsilonFstOptions())) {}

  RmEpsilonFst(const Fst<A> &fst, const RmEpsilonFstOptions &opts)
      : impl_(new RmEpsilonFstImpl<A>(fst, opts)) {}

  RmEpsilonFst(const RmEpsilonFst<A> &fst, bool reset = false) {
    if (reset) {
      impl_ = new RmEpsilonFstImpl<A>(*(fst.impl_));
    } else {
      impl_ = fst.impl_;
      impl_->IncrRefCount();
    }
  }

  virtual ~RmEpsilonFst() { if (!impl_->DecrRefCount()) delete impl_;  }

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

  virtual RmEpsilonFst<A> *Copy(bool reset = false) const {
    return new RmEpsilonFst<A>(*this, reset);
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
  RmEpsilonFstImpl<A> *Impl() { return impl_; }

 private:
  RmEpsilonFstImpl<A> *impl_;

  void operator=(const RmEpsilonFst<A> &fst);  // disallow
};


// Specialization for RmEpsilonFst.
template<class A>
class StateIterator< RmEpsilonFst<A> >
    : public CacheStateIterator< RmEpsilonFst<A> > {
 public:
  explicit StateIterator(const RmEpsilonFst<A> &fst)
      : CacheStateIterator< RmEpsilonFst<A> >(fst) {}
};


// Specialization for RmEpsilonFst.
template <class A>
class ArcIterator< RmEpsilonFst<A> >
    : public CacheArcIterator< RmEpsilonFst<A> > {
 public:
  typedef typename A::StateId StateId;

  ArcIterator(const RmEpsilonFst<A> &fst, StateId s)
      : CacheArcIterator< RmEpsilonFst<A> >(fst, s) {
    if (!fst.impl_->HasArcs(s))
      fst.impl_->Expand(s);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ArcIterator);
};


template <class A> inline
void RmEpsilonFst<A>::InitStateIterator(StateIteratorData<A> *data) const {
  data->base = new StateIterator< RmEpsilonFst<A> >(*this);
}


// Useful alias when using StdArc.
typedef RmEpsilonFst<StdArc> StdRmEpsilonFst;

}  // namespace fst

#endif  // FST_LIB_RMEPSILON_H__
