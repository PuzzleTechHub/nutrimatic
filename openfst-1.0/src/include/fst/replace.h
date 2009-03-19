// replace.h

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
// Author: johans@google.com (Johan Schalkwyk)
//
// \file
// Functions and classes for the recursive replacement of Fsts.
//

#ifndef FST_LIB_REPLACE_H__
#define FST_LIB_REPLACE_H__

#include <tr1/unordered_map>
using std::tr1::unordered_map;
#include <string>
#include <utility>
#include <vector>

#include <fst/fst.h>
#include <fst/cache.h>
#include <fst/expanded-fst.h>
#include <fst/state-table.h>
#include <fst/test-properties.h>

namespace fst {

//
// REPLACE STATE TUPLES AND TABLES
//
// The replace state table has the form
//
// template <class A, class P>
// class ReplaceStateTable {
//  public:
//   typedef A Arc;
//   typedef P PrefixId;
//   typedef typename A::StateId StateId;
//   typedef ReplaceStateTuple<StateId, PrefixId> StateTuple;
//   typedef typename A::Label Label;
//
//   // Required constuctor
//   ReplaceStateTable(const vector<pair<Label, const Fst<A>*> > &fst_tuples,
//                     Label root);
//
//   // Required copy constructor that does not copy state
//   // (as needed by fst->Copy(reset = true)).
//   ReplaceStateTable(const ReplaceStateTable<A,P> &table);
//
//   // Lookup state ID by tuple. If it doesn't exist, then add it.
//   StateId FindState(const StateTuple &tuple);
//
//   // Lookup state tuple by ID.
//   const StateTuple &Tuple(StateId id) const;
// };


// \struct ReplaceStateTuple
// \brief Tuple of information that uniquely defines a state in replace
template <class S, class P>
struct ReplaceStateTuple {
  typedef S StateId;
  typedef P PrefixId;

  ReplaceStateTuple()
      : prefix_id(-1), fst_id(kNoStateId), fst_state(kNoStateId) {}

  ReplaceStateTuple(PrefixId p, StateId f, StateId s)
      : prefix_id(p), fst_id(f), fst_state(s) {}

  PrefixId prefix_id;  // index in prefix table
  StateId fst_id;      // current fst being walked
  StateId fst_state;   // current state in fst being walked, not to be
                       // confused with the state_id of the combined fst
};


// Equality of replace state tuples.
template <class S, class P>
inline bool operator==(const ReplaceStateTuple<S, P>& x,
                       const ReplaceStateTuple<S, P>& y) {
  return x.prefix_id == y.prefix_id &&
      x.fst_id == y.fst_id &&
      x.fst_state == y.fst_state;
}


// \class ReplaceRootSelector
// Functor returning true for tuples corresponding to states in the root FST
template <class S, class P>
class ReplaceRootSelector {
 public:
  bool operator()(const ReplaceStateTuple<S, P> &tuple) const {
    return tuple.prefix_id == 0;
  }
};


// \class ReplaceFingerprint
// Fingerprint for general replace state tuples.
template <class S, class P>
class ReplaceFingerprint {
 public:
  ReplaceFingerprint(const vector<uint64> *size_array)
      : cumulative_size_array_(size_array) {}

  uint64 operator()(const ReplaceStateTuple<S, P> &tuple) const {
    return tuple.prefix_id * (cumulative_size_array_->back()) +
        cumulative_size_array_->at(tuple.fst_id - 1) +
        tuple.fst_state;
  }

 private:
  const vector<uint64> *cumulative_size_array_;
};


// \class ReplaceFstStateFingerprint
// Useful when the fst_state uniquely define the tuple.
template <class S, class P>
class ReplaceFstStateFingerprint {
 public:
  uint64 operator()(const ReplaceStateTuple<S, P>& tuple) const {
    return tuple.fst_state;
  }
};


// \class DefaultReplaceStateTable
// Default replace state table
template <class A, class P = ssize_t>
class DefaultReplaceStateTable {
 public:
  typedef A Arc;
  typedef typename A::StateId StateId;
  typedef typename A::Label Label;
  typedef P PrefixId;
  typedef ReplaceStateTuple<StateId, P> StateTuple;
  typedef VectorHashStateTable<ReplaceStateTuple<StateId, P>,
                               ReplaceRootSelector<StateId, P>,
                               ReplaceFstStateFingerprint<StateId, P>,
                               ReplaceFingerprint<StateId, P> > StateTable;

  DefaultReplaceStateTable(
      const vector<pair<Label, const Fst<A>*> > &fst_tuples,
      Label root) : root_size_(0) {
    cumulative_size_array_.push_back(0);
    for (size_t i = 0; i < fst_tuples.size(); ++i) {
      if (fst_tuples[i].first == root) {
        root_size_ = CountStates(*(fst_tuples[i].second));
        cumulative_size_array_.push_back(cumulative_size_array_.back());
      } else {
        cumulative_size_array_.push_back(cumulative_size_array_.back() +
                                         CountStates(*(fst_tuples[i].second)));
      }
    }
    state_table_ = new StateTable(
        new ReplaceRootSelector<StateId, P>,
        new ReplaceFstStateFingerprint<StateId, P>,
        new ReplaceFingerprint<StateId, P>(&cumulative_size_array_),
        root_size_,
        root_size_ + cumulative_size_array_.back());
  }

  DefaultReplaceStateTable(const DefaultReplaceStateTable<A,P> &table)
      : root_size_(table.root_size_),
        cumulative_size_array_(table.cumulative_size_array_) {
    state_table_ = new StateTable(
        new ReplaceRootSelector<StateId, P>,
        new ReplaceFstStateFingerprint<StateId, P>,
        new ReplaceFingerprint<StateId, P>(&cumulative_size_array_),
        root_size_,
        root_size_ + cumulative_size_array_.back());
  }

  ~DefaultReplaceStateTable() {
    delete state_table_;
  }

  StateId FindState(const StateTuple &tuple) {
    return state_table_->FindState(tuple);
  }

  const StateTuple &Tuple(StateId id) const {
    return state_table_->Tuple(id);
  }

 private:
  StateId root_size_;
  vector<uint64> cumulative_size_array_;
  StateTable *state_table_;
};


//
// REPLACE FST CLASS
//

// By default ReplaceFst will copy the input label of the 'replace arc'.
// For acceptors we do not want this behaviour. Instead we need to
// create an epsilon arc when recursing into the appropriate Fst.
// The epsilon_on_replace option can be used to toggle this behaviour.
template <class A, class T = DefaultReplaceStateTable<A> >
struct ReplaceFstOptions : CacheOptions {
  int64 root;    // root rule for expansion
  bool  epsilon_on_replace;
  T *state_table;

  ReplaceFstOptions(const CacheOptions &opts, int64 r)
      : CacheOptions(opts), root(r), epsilon_on_replace(false),
        state_table(0) {}
  explicit ReplaceFstOptions(int64 r)
      : root(r), epsilon_on_replace(false), state_table(0) {}
  ReplaceFstOptions(int64 r, bool epsilon_replace_arc)
      : root(r), epsilon_on_replace(epsilon_replace_arc), state_table(0) {}
  ReplaceFstOptions()
      : root(kNoLabel), epsilon_on_replace(false), state_table(0) {}
};


// \class ReplaceFstImpl
// \brief Implementation class for replace class Fst
//
// The replace implementation class supports a dynamic
// expansion of a recursive transition network represented as Fst
// with dynamic replacable arcs.
//
template <class A, class T>
class ReplaceFstImpl : public CacheImpl<A> {
 public:
  using FstImpl<A>::SetType;
  using FstImpl<A>::SetProperties;
  using FstImpl<A>::Properties;
  using FstImpl<A>::WriteHeader;
  using FstImpl<A>::SetInputSymbols;
  using FstImpl<A>::SetOutputSymbols;
  using FstImpl<A>::InputSymbols;
  using FstImpl<A>::OutputSymbols;

  using CacheImpl<A>::HasStart;
  using CacheImpl<A>::HasArcs;
  using CacheImpl<A>::SetStart;

  typedef typename A::Label   Label;
  typedef typename A::Weight  Weight;
  typedef typename A::StateId StateId;
  typedef CacheState<A> State;
  typedef A Arc;
  typedef unordered_map<Label, Label> NonTerminalHash;

  typedef T StateTable;
  typedef typename T::PrefixId PrefixId;
  typedef ReplaceStateTuple<StateId, PrefixId> StateTuple;

  // constructor for replace class implementation.
  // \param fst_tuples array of label/fst tuples, one for each non-terminal
  ReplaceFstImpl(const vector< pair<Label, const Fst<A>* > >& fst_tuples,
                 const ReplaceFstOptions<A, T> &opts)
      : CacheImpl<A>(opts),
        epsilon_on_replace_(opts.epsilon_on_replace),
        state_table_(opts.state_table ? opts.state_table :
                     new StateTable(fst_tuples, opts.root)) {

    SetType("replace");

    if (fst_tuples.size() > 0) {
      SetInputSymbols(fst_tuples[0].second->InputSymbols());
      SetOutputSymbols(fst_tuples[0].second->OutputSymbols());
    }

    bool all_negative = true;  // all nonterminals are negative?
    bool dense_range = true;   // all nonterminals are positive
                               // and form a dense range containing 1?
    for (size_t i = 0; i < fst_tuples.size(); ++i) {
      Label nonterminal = fst_tuples[i].first;
      if (nonterminal >= 0)
        all_negative = false;
      if (nonterminal > fst_tuples.size() || nonterminal <= 0)
        dense_range = false;
    }

    vector<uint64> inprops;
    bool all_ilabel_sorted = true;
    bool all_olabel_sorted = true;
    bool all_non_empty = true;
    fst_array_.push_back(0);
    for (size_t i = 0; i < fst_tuples.size(); ++i) {
      Label label = fst_tuples[i].first;
      const Fst<A> *fst = fst_tuples[i].second;
      nonterminal_hash_[label] = fst_array_.size();
      fst_array_.push_back(fst->Copy());
      if (fst->Start() == kNoStateId)
        all_non_empty = false;
      if(!fst->Properties(kILabelSorted, false))
        all_ilabel_sorted = false;
      if(!fst->Properties(kOLabelSorted, false))
        all_olabel_sorted = false;
      inprops.push_back(fst->Properties(kCopyProperties, false));
      if (i) {
        if (!CompatSymbols(InputSymbols(), fst->InputSymbols())) {
          LOG(FATAL) << "ReplaceFst::AddFst input symbols of Fst " << i
                     << " does not match input symbols of base Fst (0'th fst)";
        }
        if (!CompatSymbols(OutputSymbols(), fst->OutputSymbols())) {
          LOG(FATAL) << "ReplaceFst::AddFst output symbols of Fst " << i
                     << " does not match output symbols of base Fst "
                     << "(0'th fst)";
        }
      }
    }
    Label nonterminal = nonterminal_hash_[opts.root];
    root_ = (nonterminal > 0) ? nonterminal : 1;

    SetProperties(ReplaceProperties(inprops, root_, epsilon_on_replace_,
                                    all_non_empty));
    // We assume that all terminals are positive.  The resulting
    // ReplaceFst is known to be kILabelSorted when all sub-FSTs are
    // kILabelSorted and one of the 3 following conditions is satisfied:
    //  1. 'epsilon_on_replace' is false, or
    //  2. all non-terminals are negative, or
    //  3. all non-terninals are positive and form a dense range containing 1.
    if (all_ilabel_sorted &&
        (!epsilon_on_replace_ || all_negative || dense_range))
      SetProperties(kILabelSorted, kILabelSorted);
    // Similarly, the resulting ReplaceFst is known to be
    // kOLabelSorted when all sub-FSTs are kOLabelSorted and one of
    // the 2 following conditions is satisfied:
    //  1. all non-terminals are negative, or
    //  2. all non-terninals are positive and form a dense range containing 1.
    if (all_olabel_sorted && (all_negative || dense_range))
      SetProperties(kOLabelSorted, kOLabelSorted);
  }

  ReplaceFstImpl(const ReplaceFstImpl& impl)
      : CacheImpl<A>(impl),
        epsilon_on_replace_(impl.epsilon_on_replace_),
        state_table_(new StateTable(*(impl.state_table_))),
        nonterminal_hash_(impl.nonterminal_hash_),
        root_(impl.root_) {
    SetType("replace");
    SetProperties(impl.Properties(), kCopyProperties);
    SetInputSymbols(impl.InputSymbols());
    SetOutputSymbols(impl.OutputSymbols());
    fst_array_.reserve(impl.fst_array_.size());
    fst_array_.push_back(0);
    for (size_t i = 1; i < impl.fst_array_.size(); ++i) {
      fst_array_.push_back(impl.fst_array_[i]->Copy(true));
    }
  }

  ~ReplaceFstImpl() {
    delete state_table_;
    for (size_t i = 1; i < fst_array_.size(); ++i) {
      delete fst_array_[i];
    }
  }

  // Computes the dependency graph of the replace class and returns
  // true if the dependencies are cyclic. Cyclic dependencies will result
  // in an un-expandable replace fst.
  bool CyclicDependencies() const {
    StdVectorFst depfst;

    // one state for each fst
    for (size_t i = 1; i < fst_array_.size(); ++i)
      depfst.AddState();

    // an arc from each state (representing the fst) to the
    // state representing the fst being replaced
    for (size_t i = 1; i < fst_array_.size(); ++i) {
      for (StateIterator<Fst<A> > siter(*(fst_array_[i]));
           !siter.Done(); siter.Next()) {
        for (ArcIterator<Fst<A> > aiter(*(fst_array_[i]), siter.Value());
             !aiter.Done(); aiter.Next()) {
          const A& arc = aiter.Value();

          typename NonTerminalHash::const_iterator it =
              nonterminal_hash_.find(arc.olabel);
          if (it != nonterminal_hash_.end()) {
            Label j = it->second - 1;
            depfst.AddArc(i - 1, A(arc.olabel, arc.olabel, Weight::One(), j));
          }
        }
      }
    }

    depfst.SetStart(root_ - 1);
    depfst.SetFinal(root_ - 1, Weight::One());
    return depfst.Properties(kCyclic, true);
  }

  // Return or compute start state of replace fst
  StateId Start() {
    if (!HasStart()) {
      if (fst_array_.size() == 1) {      // no fsts defined for replace
        SetStart(kNoStateId);
        return kNoStateId;
      } else {
        const Fst<A>* fst = fst_array_[root_];
        StateId fst_start = fst->Start();
        if (fst_start == kNoStateId)  // root Fst is empty
          return kNoStateId;

        PrefixId prefix = GetPrefixId(StackPrefix());
        StateId start = state_table_->FindState(
            StateTuple(prefix, root_, fst_start));
        SetStart(start);
        return start;
      }
    } else {
      return CacheImpl<A>::Start();
    }
  }

  // return final weight of state (kInfWeight means state is not final)
  Weight Final(StateId s) {
    if (!HasFinal(s)) {
      const StateTuple& tuple  = state_table_->Tuple(s);
      const StackPrefix& stack = stackprefix_array_[tuple.prefix_id];
      const Fst<A>* fst = fst_array_[tuple.fst_id];
      StateId fst_state = tuple.fst_state;

      if (fst->Final(fst_state) != Weight::Zero() && stack.Depth() == 0)
        SetFinal(s, fst->Final(fst_state));
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

  // return the base arc iterator, if arcs have not been computed yet,
  // extend/recurse for new arcs.
  void InitArcIterator(StateId s, ArcIteratorData<A> *data) {
    if (!HasArcs(s))
      Expand(s);
    CacheImpl<A>::InitArcIterator(s, data);
  }

  // extend current state (walk arcs one level deep)
  void Expand(StateId s) {

    StateTuple tuple  = state_table_->Tuple(s);
    const Fst<A>* fst = fst_array_[tuple.fst_id];
    StateId fst_state = tuple.fst_state;
    if (fst_state == kNoStateId) {
      SetArcs(s);
      return;
    }

    // if state is final, pop up stack
    const StackPrefix& stack = stackprefix_array_[tuple.prefix_id];
    if (fst->Final(fst_state) != Weight::Zero() && stack.Depth()) {
      PrefixId prefix_id = PopPrefix(stack);
      const PrefixTuple& top = stack.Top();

      StateId nextstate = state_table_->FindState(
          StateTuple(prefix_id, top.fst_id, top.nextstate));
      AddArc(s, A(0, 0, fst->Final(fst_state), nextstate));
    }

    // extend arcs leaving the state
    for (ArcIterator< Fst<A> > aiter(*fst, fst_state);
         !aiter.Done(); aiter.Next()) {
      const Arc& arc = aiter.Value();
      if (arc.olabel == 0) {  // expand local fst
        StateId nextstate = state_table_->FindState(
            StateTuple(tuple.prefix_id, tuple.fst_id, arc.nextstate));
        AddArc(s, A(arc.ilabel, arc.olabel, arc.weight, nextstate));
      } else {
        // check for non terminal
        typename NonTerminalHash::const_iterator it =
            nonterminal_hash_.find(arc.olabel);
        if (it != nonterminal_hash_.end()) {  // recurse into non terminal
          Label nonterminal = it->second;
          const Fst<A>* nt_fst = fst_array_[nonterminal];
          PrefixId nt_prefix = PushPrefix(stackprefix_array_[tuple.prefix_id],
                                          tuple.fst_id, arc.nextstate);

          // if start state is valid replace, else arc is implicitly
          // deleted
          StateId nt_start = nt_fst->Start();
          if (nt_start != kNoStateId) {
            StateId nt_nextstate = state_table_->FindState(
                StateTuple(nt_prefix, nonterminal, nt_start));
            Label ilabel = (epsilon_on_replace_) ? 0 : arc.ilabel;
            AddArc(s, A(ilabel, 0, arc.weight, nt_nextstate));
          }
        } else {
          StateId nextstate =  state_table_->FindState(
                StateTuple(tuple.prefix_id, tuple.fst_id, arc.nextstate));
          AddArc(s, A(arc.ilabel, arc.olabel, arc.weight, nextstate));
        }
      }
    }

    SetArcs(s);
  }


  // private helper classes
 private:
  static const int kPrime0 = 7853;

  // \class PrefixTuple
  // \brief Tuple of fst_id and destination state (entry in stack prefix)
  struct PrefixTuple {
    PrefixTuple(Label f, StateId s) : fst_id(f), nextstate(s) {}

    Label   fst_id;
    StateId nextstate;
  };

  // \class StackPrefix
  // \brief Container for stack prefix.
  class StackPrefix {
   public:
    StackPrefix() {}

    // copy constructor
    StackPrefix(const StackPrefix& x) :
        prefix_(x.prefix_) {
    }

    void Push(StateId fst_id, StateId nextstate) {
      prefix_.push_back(PrefixTuple(fst_id, nextstate));
    }

    void Pop() {
      prefix_.pop_back();
    }

    const PrefixTuple& Top() const {
      return prefix_[prefix_.size()-1];
    }

    size_t Depth() const {
      return prefix_.size();
    }

   public:
    vector<PrefixTuple> prefix_;
  };


  // \class StackPrefixEqual
  // \brief Compare two stack prefix classes for equality
  class StackPrefixEqual {
   public:
    bool operator()(const StackPrefix& x, const StackPrefix& y) const {
      if (x.prefix_.size() != y.prefix_.size()) return false;
      for (size_t i = 0; i < x.prefix_.size(); ++i) {
        if (x.prefix_[i].fst_id    != y.prefix_[i].fst_id ||
           x.prefix_[i].nextstate != y.prefix_[i].nextstate) return false;
      }
      return true;
    }
  };

  //
  // \class StackPrefixKey
  // \brief Hash function for stack prefix to prefix id
  class StackPrefixKey {
   public:
    size_t operator()(const StackPrefix& x) const {
      size_t sum = 0;
      for (size_t i = 0; i < x.prefix_.size(); ++i) {
        sum += x.prefix_[i].fst_id + x.prefix_[i].nextstate*kPrime0;
      }
      return sum;
    }
  };

  typedef unordered_map<StackPrefix, PrefixId, StackPrefixKey, StackPrefixEqual>
  StackPrefixHash;

  // private methods
 private:
  // hash stack prefix (return unique index into stackprefix array)
  PrefixId GetPrefixId(const StackPrefix& prefix) {
    typename StackPrefixHash::iterator it = prefix_hash_.find(prefix);
    if (it == prefix_hash_.end()) {
      PrefixId prefix_id = stackprefix_array_.size();
      stackprefix_array_.push_back(prefix);
      prefix_hash_[prefix] = prefix_id;
      return prefix_id;
    } else {
      return it->second;
    }
  }

  // prefix id after a stack pop
  PrefixId PopPrefix(StackPrefix prefix) {
    prefix.Pop();
    return GetPrefixId(prefix);
  }

  // prefix id after a stack push
  PrefixId PushPrefix(StackPrefix prefix, Label fst_id, StateId nextstate) {
    prefix.Push(fst_id, nextstate);
    return GetPrefixId(prefix);
  }


  // private data
 private:
  // runtime options
  bool epsilon_on_replace_;

  // state table
  StateTable *state_table_;

  // cross index of unique stack prefix
  // could potentially have one copy of prefix array
  StackPrefixHash prefix_hash_;
  vector<StackPrefix> stackprefix_array_;

  NonTerminalHash nonterminal_hash_;
  vector<const Fst<A>*> fst_array_;
  Label root_;

  void operator=(const ReplaceFstImpl<A, T> &);  // disallow
};


//
// \class ReplaceFst
// \brief Recursivively replaces arcs in the root Fst with other Fsts.
// This version is a delayed Fst.
//
// ReplaceFst supports dynamic replacement of arcs in one Fst with
// another Fst. This replacement is recursive.  ReplaceFst can be used
// to support a variety of delayed constructions such as recursive
// transition networks, union, or closure.  It is constructed with an
// array of Fst(s). One Fst represents the root (or topology)
// machine. The root Fst refers to other Fsts by recursively replacing
// arcs labeled as non-terminals with the matching non-terminal
// Fst. Currently the ReplaceFst uses the output symbols of the arcs
// to determine whether the arc is a non-terminal arc or not. A
// non-terminal can be any label that is not a non-zero terminal label
// in the output alphabet.
//
// Note that the constructor uses a vector of pair<>. These correspond
// to the tuple of non-terminal Label and corresponding Fst. For example
// to implement the closure operation we need 2 Fsts. The first root
// Fst is a single Arc on the start State that self loops, it references
// the particular machine for which we are performing the closure operation.
//
template <class A, class T = DefaultReplaceStateTable<A> >
class ReplaceFst : public Fst<A> {
 public:
  friend class ArcIterator< ReplaceFst<A, T> >;
  friend class CacheStateIterator< ReplaceFst<A, T> >;
  friend class CacheArcIterator< ReplaceFst<A, T> >;

  typedef A Arc;
  typedef typename A::Label   Label;
  typedef typename A::Weight  Weight;
  typedef typename A::StateId StateId;
  typedef CacheState<A> State;

  ReplaceFst(const vector<pair<Label, const Fst<A>* > >& fst_array,
             Label root)
      : impl_(new ReplaceFstImpl<A, T>(fst_array,
                                       ReplaceFstOptions<A, T>(root))) {}

  ReplaceFst(const vector<pair<Label, const Fst<A>* > >& fst_array,
             const ReplaceFstOptions<A, T> &opts)
      : impl_(new ReplaceFstImpl<A, T>(fst_array, opts)) {}

  ReplaceFst(const ReplaceFst<A, T>& fst, bool reset = false) {
    if (reset) {
      impl_ = new ReplaceFstImpl<A, T>(*(fst.impl_));
    } else {
      impl_ = fst.impl_;
      impl_->IncrRefCount();
    }
  }

  virtual ~ReplaceFst() {
    if (!impl_->DecrRefCount()) delete impl_;
  }

  virtual StateId Start() const {
    return impl_->Start();
  }

  virtual Weight Final(StateId s) const {
    return impl_->Final(s);
  }

  virtual size_t NumArcs(StateId s) const {
    return impl_->NumArcs(s);
  }

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

  virtual const string& Type() const {
    return impl_->Type();
  }

  virtual ReplaceFst<A, T> *Copy(bool reset = false) const {
    return new ReplaceFst<A, T>(*this, reset);
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

  bool CyclicDependencies() const {
    return impl_->CyclicDependencies();
  }

 private:
  ReplaceFstImpl<A, T>* impl_;
};


// Specialization for ReplaceFst.
template<class A, class T>
class StateIterator< ReplaceFst<A, T> >
    : public CacheStateIterator< ReplaceFst<A, T> > {
 public:
  explicit StateIterator(const ReplaceFst<A, T> &fst)
      : CacheStateIterator< ReplaceFst<A, T> >(fst) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(StateIterator);
};

// Specialization for ReplaceFst.
template <class A, class T>
class ArcIterator< ReplaceFst<A, T> >
    : public CacheArcIterator< ReplaceFst<A, T> > {
 public:
  typedef typename A::StateId StateId;

  ArcIterator(const ReplaceFst<A, T> &fst, StateId s)
      : CacheArcIterator< ReplaceFst<A, T> >(fst, s) {
    if (!fst.impl_->HasArcs(s))
      fst.impl_->Expand(s);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ArcIterator);
};

template <class A, class T> inline
void ReplaceFst<A, T>::InitStateIterator(StateIteratorData<A> *data) const {
  data->base = new StateIterator< ReplaceFst<A, T> >(*this);
}

typedef ReplaceFst<StdArc> StdReplaceFst;


// // Recursivively replaces arcs in the root Fst with other Fsts.
// This version writes the result of replacement to an output MutableFst.
//
// Replace supports replacement of arcs in one Fst with another
// Fst. This replacement is recursive.  Replace takes an array of
// Fst(s). One Fst represents the root (or topology) machine. The root
// Fst refers to other Fsts by recursively replacing arcs labeled as
// non-terminals with the matching non-terminal Fst. Currently Replace
// uses the output symbols of the arcs to determine whether the arc is
// a non-terminal arc or not. A non-terminal can be any label that is
// not a non-zero terminal label in the output alphabet.  Note that
// input argument is a vector of pair<>. These correspond to the tuple
// of non-terminal Label and corresponding Fst.
template<class Arc>
void Replace(const vector<pair<typename Arc::Label,
             const Fst<Arc>* > >& ifst_array,
             MutableFst<Arc> *ofst, typename Arc::Label root,
             bool epsilon_on_replace) {
  ReplaceFstOptions<Arc> opts(root, epsilon_on_replace);
  opts.gc_limit = 0;  // Cache only the last state for fastest copy.
  *ofst = ReplaceFst<Arc>(ifst_array, opts);
}

template<class Arc>
void Replace(const vector<pair<typename Arc::Label,
             const Fst<Arc>* > >& ifst_array,
             MutableFst<Arc> *ofst, typename Arc::Label root) {
  Replace(ifst_array, ofst, root, false);
}

}

#endif  // FST_LIB_REPLACE_H__
