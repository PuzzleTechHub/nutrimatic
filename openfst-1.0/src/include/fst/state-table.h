// state-table.h

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
// Classes for representing the mapping between state tuples and state Ids.

#include <deque>
#include <vector>


#ifndef FST_LIB_STATE_TABLE_H__
#define FST_LIB_STATE_TABLE_H__

#include <fst/expanded-fst.h>

namespace fst {

// STATE TABLES - these determine the bijective mapping between state
// tuples (e.g. in composition triples of two FST states and a
// composition filter state) and their corresponding state IDs.
// They are classes, templated on state tuples, of the form:
//
// template <class T>
// class StateTable {
//  public:
//   typedef typename T Tuple;
//
//   // Required constructors.
//   StateTable();

//   // Lookup state ID by tuple. If it doesn't exist, then add it.
//   StateId FindState(const StateTuple &);
//   // Lookup state tuple by state ID.
//   const StateTuple<StateId> &Tuple(StateId) const;
// };
//
// A state tuple has the form:
//
// template <class S>
// struct StateTuple {
//   typedef typename S StateId;
//
//   // Required constructor.
//   StateTuple();
// };


// An implementation using a hash map for the tuple to state ID mapping.
// The state tuple T must have == defined and the default constructor
// must produce a tuple that will never be seen. H is the hash function.
template <class T, class H>
class HashStateTable {
 public:
  typedef T StateTuple;
  typedef typename StateTuple::StateId StateId;

  HashStateTable() {
    StateTuple empty_tuple;
  }

  StateId FindState(const StateTuple &tuple) {
    StateId &id_ref = tuple2id_[tuple];
    if (id_ref == 0) {  // Tuple not found; store and assign it a new ID.
      id2tuple_.push_back(tuple);
      id_ref = id2tuple_.size();
    }
    return id_ref - 1;  // NB: id_ref = ID + 1
  }

  const StateTuple &Tuple(StateId s) const {
    return id2tuple_[s];
  }

 private:
  unordered_map<StateTuple, StateId, H> tuple2id_;
  vector<StateTuple> id2tuple_;

  DISALLOW_COPY_AND_ASSIGN(HashStateTable);
};


// An implementation using a hash set for the tuple to state ID
// mapping.  The hash set holds 'keys' which are either the state ID
// or kCurrentKey.  These keys can be mapped to tuples either by
// looking up in the tuple vector or, if kCurrentKey, in current_tuple_
// member. The hash and key equality functions map to tuples first.
// The state tuple T must have == defined and the default constructor
// must produce a tuple that will never be seen. H is the hash
// function.
template <class T, class H>
class CompactHashStateTable {
 public:
  friend class HashFunc;
  friend class HashEqual;
  typedef T StateTuple;
  typedef typename StateTuple::StateId StateId;
  typedef StateId Key;

  CompactHashStateTable()
      : hash_func_(*this),
        hash_equal_(*this),
        keys_(0, hash_func_, hash_equal_) {
  }

  StateId FindState(const StateTuple &tuple) {
    current_tuple_ = &tuple;
    typename KeyHashSet::const_iterator it = keys_.find(kCurrentKey);
    if (it == keys_.end()) {
      Key key = id2tuple_.size();
      id2tuple_.push_back(tuple);
      keys_.insert(key);
      return key;
    } else {
      return *it;
    }
  }

  const StateTuple &Tuple(StateId s) const {
    return id2tuple_[s];
  }

 private:
  static const StateId kEmptyKey;
  static const StateId kCurrentKey;

  class HashFunc {
   public:
    HashFunc(const CompactHashStateTable &ht) : ht_(&ht) {}

    size_t operator()(Key k) const { return hf(ht_->Key2Tuple(k)); }
   private:
    const CompactHashStateTable *ht_;
    H hf;
  };

  class HashEqual {
   public:
    HashEqual(const CompactHashStateTable &ht) : ht_(&ht) {}

    bool operator()(Key k1, Key k2) const {
      return ht_->Key2Tuple(k1) == ht_->Key2Tuple(k2);
    }
   private:
    const CompactHashStateTable *ht_;
  };

  typedef unordered_set<Key, HashFunc, HashEqual> KeyHashSet;

  const StateTuple &Key2Tuple(Key k) const {
    if (k == kEmptyKey)
      return empty_tuple_;
    else if (k == kCurrentKey)
      return *current_tuple_;
    else
      return id2tuple_[k];
  }

  HashFunc hash_func_;
  HashEqual hash_equal_;
  KeyHashSet keys_;
  vector<StateTuple> id2tuple_;
  const StateTuple empty_tuple_;
  const StateTuple *current_tuple_;

  DISALLOW_COPY_AND_ASSIGN(CompactHashStateTable);
};

template <class T, class H>
const typename CompactHashStateTable<T, H>::StateId
CompactHashStateTable<T, H>::kEmptyKey = -1;

template <class T, class H>
const typename CompactHashStateTable<T, H>::StateId
CompactHashStateTable<T, H>::kCurrentKey = -2;


// An implementation using a vector for the tuple to state mapping.
// It is passed a function object FP that should fingerprint tuples
// uniquely to an integer that can used as a vector index. Normally,
// VectorStateTable constructs the FP object.  The user can instead
// pass in this object; in that case, VectorStateTable takes its
// ownership.
template <class T, class FP>
class VectorStateTable {
 public:
  typedef T StateTuple;
  typedef typename StateTuple::StateId StateId;
  typedef typename StateTuple::FilterState FilterState;

  explicit VectorStateTable(FP *fp = 0)
      : fp_(fp ? fp : new FP()) {}

  ~VectorStateTable() { delete fp_; }

  StateId FindState(const StateTuple &tuple) {
    ssize_t fp = (*fp_)(tuple);
    if (fp >= fp2id_.size())
      fp2id_.resize(fp + 1);
    StateId &id_ref = fp2id_[fp];
    if (id_ref == 0) {  // Tuple not found; store and assign it a new ID.
      id2tuple_.push_back(tuple);
      id_ref = id2tuple_.size();
    }
    return id_ref - 1;  // NB: id_ref = ID + 1
  }

  const StateTuple &Tuple(StateId s) const {
    return id2tuple_[s];
  }

  const FP &Fingerprint() const { return *fp_; }

 private:
  FP *fp_;
  vector<StateId> fp2id_;
  vector<StateTuple> id2tuple_;

  DISALLOW_COPY_AND_ASSIGN(VectorStateTable);
};

// An implementation using a hash map for the tuple to state ID
// mapping. This version permits erasing of states.  The state tuple T
// must have == defined and the default constructor must produce a
// tuple that will never be seen. F is the hash function.
template <class T, class F>
class ErasableStateTable {
 public:
  typedef T StateTuple;
  typedef typename StateTuple::StateId StateId;

  ErasableStateTable() : first_(0) {}

  StateId FindState(const StateTuple &tuple) {
    StateId &id_ref = tuple2id_[tuple];
    if (id_ref == 0) {  // Tuple not found; store and assign it a new ID.
      id2tuple_.push_back(tuple);
      id_ref = id2tuple_.size() + first_;
    }
    return id_ref - 1;  // NB: id_ref = ID + 1
  }

  const StateTuple &Tuple(StateId s) const {
    return id2tuple_[s - first_];
  }

  void Erase(StateId s) {
    StateTuple &tuple = id2tuple_[s - first_];
    typename unordered_map<StateTuple, StateId, F>::iterator it =
        tuple2id_.find(tuple);
    tuple2id_.erase(it);
    id2tuple_[s - first_] = empty_tuple_;
    while (id2tuple_.front() == empty_tuple_) {
      id2tuple_.pop_front();
      ++first_;
    }
  }

 private:
  unordered_map<StateTuple, StateId, F> tuple2id_;
  deque<StateTuple> id2tuple_;
  StateTuple empty_tuple_;
  StateId first_;        // StateId of first element in the deque;

  DISALLOW_COPY_AND_ASSIGN(ErasableStateTable);
};


// An implementation using a vector and a compact hash table. The
// selecting functor S returns true for tuples to be hashed in the
// vector.  The fingerprinting functor FP returns a unique fingerprint
// for each tuple to be hashed in the vector (these need to be
// suitable for indexing in a vector).  The hash functor H is used when
// hashing tuple into the compact hash table.
template <class T, class S, class FP, class H>
class VectorHashStateTable {
 public:
  friend class HashFunc;
  friend class HashEqual;
  typedef T StateTuple;
  typedef typename StateTuple::StateId StateId;
  typedef StateId Key;

  VectorHashStateTable(S *s, FP *fp, H *h,
                       size_t vector_size = 0,
                       size_t tuple_size = 0)
      : selector_(s),
        fp_(fp),
        h_(h),
        hash_func_(*this),
        hash_equal_(*this),
        keys_(0, hash_func_, hash_equal_) {
    if (vector_size)
      fp2id_.reserve(vector_size);
    if (tuple_size)
      id2tuple_.reserve(tuple_size);
  }

  ~VectorHashStateTable() {
    delete selector_;
    delete fp_;
    delete h_;
  }

  StateId FindState(const StateTuple &tuple) {
    if ((*selector_)(tuple)) {  // Use the vector if 'selector_(tuple) == true'
      uint64 fp = (*fp_)(tuple);
      if (fp2id_.size() <= fp)
        fp2id_.resize(fp + 1, 0);
      if (fp2id_[fp] == 0) {
        id2tuple_.push_back(tuple);
        fp2id_[fp] = id2tuple_.size();
      }
      return fp2id_[fp] - 1;  // NB: assoc_value = ID + 1
    } else {  // Use the hash table otherwise.
      current_tuple_ = &tuple;
      typename KeyHashSet::const_iterator it = keys_.find(kCurrentKey);
      if (it == keys_.end()) {
        Key key = id2tuple_.size();
        id2tuple_.push_back(tuple);
        keys_.insert(key);
        return key;
      } else {
        return *it;
      }
    }
  }

  const StateTuple &Tuple(StateId s) const {
    return id2tuple_[s];
  }

  const S &Selector() const { return *selector_; }

  const FP &Fingerprint() const { return *fp_; }

  const H &Hash() const { return *h_; }

 private:
  static const StateId kEmptyKey;
  static const StateId kCurrentKey;

  class HashFunc {
   public:
    HashFunc(const VectorHashStateTable &ht) : ht_(&ht) {}

    size_t operator()(Key k) const { return (*(ht_->h_))(ht_->Key2Tuple(k)); }
   private:
    const VectorHashStateTable *ht_;
  };

  class HashEqual {
   public:
    HashEqual(const VectorHashStateTable &ht) : ht_(&ht) {}

    bool operator()(Key k1, Key k2) const {
      return ht_->Key2Tuple(k1) == ht_->Key2Tuple(k2);
    }
   private:
    const VectorHashStateTable *ht_;
  };

  typedef unordered_set<Key, HashFunc, HashEqual> KeyHashSet;

  const StateTuple &Key2Tuple(Key k) const {
    if (k == kEmptyKey)
      return empty_tuple_;
    else if (k == kCurrentKey)
      return *current_tuple_;
    else
      return id2tuple_[k];
  }


  S *selector_;  // Returns true if tuple hashed into vector
  FP *fp_;       // Fingerprint used when hashing tuple into vector
  H *h_;         // Hash function used when hashing tuple into hash_set

  vector<StateTuple> id2tuple_;  // Maps state IDs to tuple
  vector<StateId> fp2id_;        // Maps tuple fingerprints to IDs

  // Compact implementation of the hash table mapping tuples to
  // state IDs using the hash function 'h_'
  HashFunc hash_func_;
  HashEqual hash_equal_;
  KeyHashSet keys_;
  const StateTuple empty_tuple_;
  const StateTuple *current_tuple_;

  DISALLOW_COPY_AND_ASSIGN(VectorHashStateTable);
};

template <class T, class S, class FP, class H>
const typename VectorHashStateTable<T, S, FP, H>::StateId
VectorHashStateTable<T, S, FP, H>::kEmptyKey = -1;

template <class T, class S, class FP, class H>
const typename VectorHashStateTable<T, S, FP, H>::StateId
VectorHashStateTable<T, S, FP, H>::kCurrentKey = -2;


//
// COMPOSITION STATE TUPLES AND TABLES
//
// The composition state table has the form:
//
// template <class A, class F>
// class ComposeStateTable {
//  public:
//   typedef A Arc;
//   typedef F FilterState;
//   typedef typename A::StateId StateId;
//   typedef ComposeStateTuple<StateId> StateTuple;
//
//   // Required constructors. Copy constructor does not
//   // copy state (as needed by fst->Copy(reset = true)).
//   ComposeStateTable(const Fst<Arc> &fst1, const Fst<Arc> &fst2);
//   ComposeStateTable(const ComposeStateTable<A, F> &table);
//   // Lookup state ID by tuple. If it doesn't exist, then add it.
//   StateId FindState(const StateTuple &);
//   // Lookup state tuple by state ID.
//   const StateTuple<StateId> &Tuple(StateId) const;
// };

// Represents the composition state.
template <typename S, typename F>
struct ComposeStateTuple {
  typedef S StateId;
  typedef F FilterState;

  ComposeStateTuple()
      : state_id1(kNoLabel), state_id2(kNoLabel),
        filter_state(FilterState::NoState()) {}

  ComposeStateTuple(StateId s1, StateId s2, const FilterState &f)
      : state_id1(s1), state_id2(s2), filter_state(f) {}

  StateId state_id1;          // State Id on fst1
  StateId state_id2;          // State Id on fst2
  FilterState filter_state;  // State of composition filter
};

// Equality of composition state tuples.
template <typename S, typename F>
inline bool operator==(const ComposeStateTuple<S, F>& x,
                       const ComposeStateTuple<S, F>& y) {
  return x.state_id1 == y.state_id1 &&
      x.state_id2 == y.state_id2 &&
      x.filter_state == y.filter_state;
}


// Hashing of composition state tuples.
template <typename S, typename F>
class ComposeHash {
 public:
  size_t operator()(const ComposeStateTuple<S, F>& t) const {
    return static_cast<size_t>(t.state_id1 +
                               t.state_id2 * kPrime0 +
                               t.filter_state.Hash() * kPrime1);
  }
 private:
  static const int kPrime0;
  static const int kPrime1;
};

template <typename S, typename F>
const int ComposeHash<S, F>::kPrime0 = 7853;

template <typename S, typename F>
const int ComposeHash<S, F>::kPrime1 = 7867;


// A HashStateTable over composition tuples.
template <typename A,
          typename F,
          typename H =
          CompactHashStateTable<ComposeStateTuple<typename A::StateId, F>,
                                ComposeHash<typename A::StateId, F> > >
class GenericComposeStateTable : public H {
 public:
  typedef A Arc;
  typedef typename A::StateId StateId;
  typedef F FilterState;
  typedef ComposeStateTuple<StateId, F> StateTuple;

  GenericComposeStateTable(const Fst<A> &fst1, const Fst<A> &fst2) {}

  GenericComposeStateTable(const GenericComposeStateTable<A, F> &table) {}

 private:
  void operator=(const GenericComposeStateTable<A, F> &table);  // disallow
};


//  Fingerprint for general composition tuples.
template  <typename S, typename F>
class ComposeFingerprint {
 public:
  typedef S StateId;
  typedef F FilterState;
  typedef ComposeStateTuple<S, F> StateTuple;

  // Required, but here useless, constructor.
  ComposeFingerprint() {
    LOG(FATAL) << "TupleFingerprint: # of FST state must be provided.";
  }

  // Constructor is provided the sizes of the input FSTs
  ComposeFingerprint(StateId nstates1, StateId nstates2)
      : mult1_(nstates1), mult2_(nstates1 * nstates2) { }

  size_t operator()(const StateTuple &tuple) {
    return tuple.state_id1 + tuple.state_id2 * mult1_ +
        tuple.filter_state.Hash() * mult2_;
  }

 private:
  ssize_t mult1_;
  ssize_t mult2_;
};


// Useful when the first composition state determines the tuple.
template  <typename S, typename F>
class ComposeState1Fingerprint {
 public:
  typedef S StateId;
  typedef F FilterState;
  typedef ComposeStateTuple<S, F> StateTuple;

  size_t operator()(const StateTuple &tuple) { return tuple.state_id1; }
};


// Useful when the second composition state determines the tuple.
template  <typename S, typename F>
class ComposeState2Fingerprint {
 public:
  typedef S StateId;
  typedef F FilterState;
  typedef ComposeStateTuple<S, F> StateTuple;

  size_t operator()(const StateTuple &tuple) { return tuple.state_id2; }
};


// A VectorStateTable over composition tuples.  This can be used when
// the product of number of states in FST1 and FST2 (and the
// composition filter state hash) is manageable. If the FSTs are not
// expanded Fsts, they will first have their states counted.
template  <typename A, typename F>
class ProductComposeStateTable : public
VectorStateTable<ComposeStateTuple<typename A::StateId, F>,
                 ComposeFingerprint<typename A::StateId, F> > {
 public:
  typedef A Arc;
  typedef typename A::StateId StateId;
  typedef F FilterState;
  typedef ComposeStateTuple<StateId, F> StateTuple;

  ProductComposeStateTable(const Fst<A> &fst1, const Fst<A> &fst2)
      : VectorStateTable<ComposeStateTuple<StateId, F>,
                         ComposeFingerprint<StateId, F> >
  (new ComposeFingerprint<StateId, F>(CountStates(fst1),
                                      CountStates(fst2))) { }

  ProductComposeStateTable(const ProductComposeStateTable<A, F> &table)
      : VectorStateTable<ComposeStateTuple<StateId, F>,
                         ComposeFingerprint<StateId, F> >
        (new ComposeFingerprint<StateId, F>(table.Fingerprint())) {}

 private:
  void operator=(const ProductComposeStateTable<A, F> &table);  // disallow
};

// A VectorStateTable over composition tuples.  This can be used when
// FST1 is a string (satisfies kStringProperties) and FST2 is
// epsilon-free and deterministic. It should be used with a
// composition filter that creates at most one filter state per tuple
// under these conditions (e.g. SequenceComposeFilter or
// MatchComposeFilter).
template <typename A, typename F>
class StringDetComposeStateTable : public
VectorStateTable<ComposeStateTuple<typename A::StateId, F>,
                 ComposeState1Fingerprint<typename A::StateId, F> > {
 public:
  typedef A Arc;
  typedef typename A::StateId StateId;
  typedef F FilterState;
  typedef ComposeStateTuple<StateId, F> StateTuple;

  StringDetComposeStateTable(const Fst<A> &fst1, const Fst<A> &fst2) {
    uint64 props1 = kString;
    uint64 props2 = kIDeterministic | kNoIEpsilons;
    if (fst1.Properties(props1, true) != props1 ||
        fst2.Properties(props2, true) != props2)
      LOG(FATAL) << "StringDetComposeStateTable: fst1 not a string or"
                 << " fst2 not input deterministic and epsilon-free";
  }

  StringDetComposeStateTable(const StringDetComposeStateTable<A, F> &table) {}

 private:
  void operator=(const StringDetComposeStateTable<A, F> &table);  // disallow
};


// A VectorStateTable over composition tuples.  This can be used when
// FST2 is a string (satisfies kStringProperties) and FST1 is
// epsilon-free and deterministic. It should be used with a
// composition filter that creates at most one filter state per tuple
// under these conditions (e.g. SequenceComposeFilter or
// MatchComposeFilter).
template <typename A, typename F>
class DetStringComposeStateTable : public
VectorStateTable<ComposeStateTuple<typename A::StateId, F>,
                 ComposeState1Fingerprint<typename A::StateId, F> > {
 public:
  typedef A Arc;
  typedef typename A::StateId StateId;
  typedef F FilterState;
  typedef ComposeStateTuple<StateId, F> StateTuple;

  DetStringComposeStateTable(const Fst<A> &fst1, const Fst<A> &fst2) {
    uint64 props1 = kODeterministic | kNoOEpsilons;
    uint64 props2 = kString;
    if (fst1.Properties(props1, true) != props1 ||
        fst2.Properties(props2, true) != props2)
      LOG(FATAL) << "StringDetComposeStateTable: fst2 not a string or"
                 << " fst1 not output deterministic and epsilon-free";
  }

  DetStringComposeStateTable(const DetStringComposeStateTable<A, F> &table) {}

 private:
  void operator=(const DetStringComposeStateTable<A, F> &table);  // disallow
};


// An ErasableStateTable over composition tuples. The Erase(StateId) method
// can be called if the user either is sure that composition will never return
// to that tuple or doesn't care that if it does, it is assigned a new
// state ID.
template <typename A, typename F>
class ErasableComposeStateTable : public
ErasableStateTable<ComposeStateTuple<typename A::StateId, F>,
                   ComposeHash<typename A::StateId, F> > {
 public:
  typedef A Arc;
  typedef typename A::StateId StateId;
  typedef F FilterState;
  typedef ComposeStateTuple<StateId, F> StateTuple;

  ErasableComposeStateTable(const Fst<A> &fst1, const Fst<A> &fst2) {}

  ErasableComposeStateTable(const ErasableComposeStateTable<A, F> &table) {}

 private:
  void operator=(const ErasableComposeStateTable<A, F> &table);  // disallow
};


}  // namespace fst


#endif  // FST_LIB_STATE_TABLE_H__
