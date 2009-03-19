// compact-fst.h


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
// FST Class for memory-efficient representation of common types of
// FSTs: linear automata, acceptors, unweighted FSTs, ...

#ifndef FST_LIB_COMPACT_FST_H__
#define FST_LIB_COMPACT_FST_H__

#include <iterator>
#include <utility>
#include <vector>

#include <fst/cache.h>
#include <fst/expanded-fst.h>
#include <fst/fst-decl.h>  // For optional argument declarations
#include <fst/test-properties.h>
#include <fst/util.h>

namespace fst {

struct CompactFstOptions : public CacheOptions {
  // CompactFst default caching behaviour is to do no caching. Most
  // compactors are cheap and therefore we save memory by not doing
  // caching.
  CompactFstOptions() : CacheOptions(true, 0) {}
  CompactFstOptions(const CacheOptions &opts) : CacheOptions(opts) {}
};

// Compactor Interface - class determinies how arcs and final weights
// are compacted and expanded.
//
// Final weights are treated as transitions to the superfinal state,
// i.e. ilabel = olabel = kNoLabel and nextstate = kNoStateId.
//
// There are two types of compactors:
//
// * Fixed out-degree compactors: 'compactor.Size()' returns a
// positive integer 's'. An FST can be compacted by this compactor
// only if each state has exactly 's' outgoing transitions (counting a
// non-Zero() final weight as a transition). A typical example is a
// compactor for string FSTs, i.e. 's == 1'.
//
// * Variable out-degree compactors: 'compactor.Size() == -1'. There
// are no out-degree restrictions for these compactors.
//
//
// class Compactor {
//  public:
//   // Element is the type of the compacted transitions.
//   typedef ... Element;
//   // Return the compacted representation of a transition 'arc'
//   // at a state 's'.
//   Element Compact(StateId s, const Arc &arc);
//   // Return the transition at state 's' represented by the compacted
//   // transition 'e'.
//   Arc Expand(StateId s, const Element &e);
//   // Return -1 for variable out-degree compactors, and the mandatory
//   // out-degree otherwise.
//   ssize_t Size();
//   // Test whether 'fst' can be compacted by this compactor.
//   bool Compatible(const Fst<A> &fst);
//   // Return the properties that are always true for an fst
//   // compacted using this compactor
//   uint64 Properties();
//   // Return a string identifying the type of compactor.
//   static const string &Type();
//   // Write a compactor to a file.
//   bool Write(ostream &strm);
//   // Read a compactor from a file.
//   static Compactor *Read(istream &strm);
//   // Default constructor (optional, see comment below).
//   Compactor();
// };
//
// The default constructor is only required for FST_REGISTER to work
// (i.e. enabling Convert() and the command-line utilities to work
// with this new compactor). However, a default constructor always
// needs to be specify for this code to compile, but one can have it
// simply raised an error when called:
//
// Compactor::Compactor() {
//   LOG(FATAL) << "Compactor: no default constructor";
// }


// Implementation data for Compact Fst, which can shared between otherwise
// independent copies.
//
// The implementation contains two arrays: 'states_' and 'compacts_'.
//
// For fixed out-degree compactors, the 'states_' array is unallocated.
// The 'compacts_' contains the compacted transitions. Its size is
// 'ncompacts_'. The outgoing transitions at a given state are stored
// consecutively. For a given state 's', its 'compactor.Size()' outgoing
// transitions (including superfinal transition when 's' is final), are
// stored in position ['s*compactor.Size()', '(s+1)*compactor_.Size()').
//
// For variable out-degree compactors, the states_ array has size
// 'nstates_ + 1' and contains pointers to positions into 'compacts_'.
// For a given state 's', the compacted transitions of 's' are
// stored in positions [ 'states_[s]', 'states_[s + 1]' ) in 'compacts_'.
// By convention, 'states_[nstates_] == ncompacts_'.
//
// In both cases, the superfinal transitons (when 's' is final, i.e.
// 'Final(s) != Weight::Zero()') is stored first.
//
// The unsigned type U is used to represent indices into the compacts_
// array.
template <class A, class C, class U>
class CompactFstData {
  public:
  typedef A Arc;
  typedef typename A::Weight Weight;
  typedef typename A::StateId StateId;
  typedef C Compactor;
  typedef typename C::Element CompactElement;
  typedef U Unsigned;

  CompactFstData()
      : states_(0),
        compacts_(0),
        nstates_(0),
        ncompacts_(0),
        narcs_(0),
        start_(kNoStateId) {}

  CompactFstData(const Fst<A> &fst, const Compactor &compactor);

  template <class Iterator>
  CompactFstData(const Iterator &begin, const Iterator &end,
                 const Compactor &compactor);

  ~CompactFstData() {
    delete[] states_;
    delete[] compacts_;
  }

  static CompactFstData<A, C, U> *Read(istream &strm,
                                       const FstReadOptions &opts,
                                       const FstHeader &hdr,
                                       const Compactor &compactor);

  bool Write(ostream &strm, const FstWriteOptions &opts,
             const C &compactor) const;

  Unsigned States(StateId i) const { return states_[i]; }
  const CompactElement &Compacts(size_t i) const { return compacts_[i]; }
  StateId NumStates() const { return nstates_; }
  size_t NumCompacts() const { return ncompacts_; }
  size_t NumArcs() const { return narcs_; }
  StateId Start() const { return start_; }

  int RefCount() const { return ref_count_.count(); }
  int IncrRefCount() { return ref_count_.Incr(); }
  int DecrRefCount() { return ref_count_.Decr(); }

 private:
  // Byte alignment for states and arcs in file format
  static const int kFileAlign = 16;

  Unsigned *states_;
  CompactElement *compacts_;
  StateId nstates_;
  size_t ncompacts_;
  size_t narcs_;
  StateId start_;
  RefCounter ref_count_;
};

template <class A, class C, class U>
CompactFstData<A, C, U>::CompactFstData(const Fst<A> &fst,
                                     const C &compactor)
    : states_(0),
      compacts_(0),
      nstates_(0),
      ncompacts_(0),
      narcs_(0),
      start_(kNoStateId) {
  start_ = fst.Start();
  // Count # of states and arcs.
  StateId nfinals = 0;
  for (StateIterator< Fst<A> > siter(fst);
       !siter.Done();
       siter.Next()) {
    ++nstates_;
    StateId s = siter.Value();
    for (ArcIterator< Fst<A> > aiter(fst, s);
         !aiter.Done();
         aiter.Next())
      ++narcs_;
    if (fst.Final(s) != Weight::Zero()) ++nfinals;
  }
  if (compactor.Size() == -1) {
    states_ = new Unsigned[nstates_ + 1];
    ncompacts_ = narcs_ + nfinals;
    compacts_ = new CompactElement[ncompacts_];
    states_[nstates_] = ncompacts_;
  } else {
    states_ = 0;
    ncompacts_ = nstates_ * compactor.Size();
    if ((narcs_ + nfinals) != ncompacts_)
      LOG(FATAL) << "CompactFstData: compactor incompatible with fst";
    compacts_ = new CompactElement[ncompacts_];
  }
  size_t pos = 0, fpos = 0;
  for (StateId s = 0; s < nstates_; ++s) {
    fpos = pos;
    if (compactor.Size() == -1)
      states_[s] = pos;
    if (fst.Final(s) != Weight::Zero())
      compacts_[pos++] = compactor.Compact(s, Arc(kNoLabel, kNoLabel,
                                                  fst.Final(s), kNoStateId));
    for (ArcIterator< Fst<A> > aiter(fst, s);
         !aiter.Done();
         aiter.Next())
      compacts_[pos++] = compactor.Compact(s, aiter.Value());
    if ((compactor.Size() != -1) && ((pos - fpos) != compactor.Size()))
      LOG(FATAL) << "CompactFstData: compactor incompatible with fst";
  }
  if (pos != ncompacts_)
      LOG(FATAL) << "CompactFstData: compactor incompatible with fst";
}

template <class A, class C, class U>
template <class Iterator>
CompactFstData<A, C, U>::CompactFstData(const Iterator &begin,
                                        const Iterator &end,
                                        const C &compactor)
    : states_(0),
      compacts_(0),
      nstates_(0),
      ncompacts_(0),
      narcs_(0),
      start_(kNoStateId) {
  // For strings, allow implicit final weight. Empty input is the empty string.
  if (compactor.Size() != -1) {
    ncompacts_ = distance(begin, end);
    if (compactor.Size() == 1) {
      if (ncompacts_ == 0) {
        ++ncompacts_;
      } else {
        A arc = compactor.Expand(ncompacts_ - 1,
                                      *(begin + (ncompacts_ - 1)));
        if (arc.ilabel != kNoLabel)
          ++ncompacts_;
      }
    }
    if (ncompacts_ % compactor.Size())
      LOG(FATAL) << "CompactFstData: size of input container incompatible"
                 << " with compactor";
    if (ncompacts_ == 0)
      return;
    start_ = 0;
    nstates_ = ncompacts_ / compactor.Size();
    compacts_ = new CompactElement[ncompacts_];
    size_t i = 0;
    Iterator it = begin;
    for(; it != end; ++it, ++i){
      compacts_[i] = *it;
      if (compactor.Expand(i, *it).ilabel != kNoLabel)
        ++narcs_;
    }
    if (i < ncompacts_)
      compacts_[i] = compactor.Compact(i, A(kNoLabel, kNoLabel,
                                              Weight::One(), kNoStateId));
  } else {
    if (distance(begin, end) == 0)
      return;
    // Count # of states, arcs and compacts.
    Iterator it = begin;
    for(size_t i = 0; it != end; ++it, ++i) {
      A arc = compactor.Expand(i, *it);
      if (arc.ilabel != kNoLabel) {
        ++narcs_;
        ++ncompacts_;
      } else {
        ++nstates_;
        if (arc.weight != Weight::Zero())
          ++ncompacts_;
      }
    }
    start_ = 0;
    compacts_ = new CompactElement[ncompacts_];
    states_ = new Unsigned[nstates_ + 1];
    states_[nstates_] = ncompacts_;
    size_t i = 0, s = 0;
    for(it = begin; it != end; ++it) {
      A arc = compactor.Expand(i, *it);
      if (arc.ilabel != kNoLabel) {
        compacts_[i++] = *it;
      } else {
        states_[s++] = i;
        if (arc.weight != Weight::Zero())
          compacts_[i++] = *it;
      }
    }
    if ((s != nstates_) || (i != ncompacts_))
      LOG(FATAL) << "CompactFstData: ill-formed input container";
  }
}

template<class A, class C, class U>
CompactFstData<A, C, U> *CompactFstData<A, C, U>::Read(
    istream &strm,
    const FstReadOptions &opts,
    const FstHeader &hdr,
    const C &compactor) {
  CompactFstData<A, C, U> *data = new CompactFstData<A, C, U>();
  data->start_ = hdr.Start();
  data->nstates_ = hdr.NumStates();
  data->narcs_ = hdr.NumArcs();

  if (compactor.Size() == -1) {
    data->states_ = new Unsigned[data->nstates_ + 1];
    char c;
    for (int i = 0; i < kFileAlign && strm.tellg() % kFileAlign; ++i)
      strm.read(&c, 1);
    // TODO: memory map this
    size_t b = (data->nstates_ + 1) * sizeof(Unsigned);
    strm.read(reinterpret_cast<char *>(data->states_), b);
    if (!strm) {
      LOG(ERROR) << "CompactFst::Read: Read failed: " << opts.source;
      return 0;
    }
  } else {
    data->states_ = 0;
  }
  data->ncompacts_ = compactor.Size() == -1
      ? data->states_[data->nstates_]
      : data->nstates_ * compactor.Size();
  data->compacts_ = new CompactElement[data->ncompacts_];
  // TODO: memory map this
  char c;
  size_t b = data->ncompacts_ * sizeof(CompactElement);
  for (int i = 0; i < kFileAlign && strm.tellg() % kFileAlign; ++i)
    strm.read(&c, 1);
  strm.read(reinterpret_cast<char *>(data->compacts_), b);
  if (!strm) {
    LOG(ERROR) << "CompactFst::Read: Read failed: " << opts.source;
    return 0;
  }
  return data;
}

template<class A, class C, class U>
bool CompactFstData<A, C, U>::Write(ostream &strm,
                                 const FstWriteOptions &opts,
                                 const C &compactor) const {
  if (states_) {
    for (int i = 0; i < kFileAlign && strm.tellp() % kFileAlign; ++i)
      strm.write("", 1);
    strm.write(reinterpret_cast<char *>(states_),
               (nstates_ + 1) * sizeof(Unsigned));
  }
  for (int i = 0; i < kFileAlign && strm.tellp() % kFileAlign; ++i)
    strm.write("", 1);
  strm.write(reinterpret_cast<char *>(compacts_),
             ncompacts_ * sizeof(CompactElement));

  strm.flush();
  if (!strm) {
    LOG(ERROR) << "CompactFst::Write: Write failed: " << opts.source;
    return false;
  }
  return true;
}

template <class A, class C, class U> class CompactFst;
template <class F, class G> void Cast(const F &, G *);

// Implementation class for CompactFst, which contains CompactFstData
// and Fst cache.
template <class A, class C, class U>
class CompactFstImpl : public CacheImpl<A> {
 public:
  using FstImpl<A>::SetType;
  using FstImpl<A>::SetProperties;
  using FstImpl<A>::Properties;
  using FstImpl<A>::SetInputSymbols;
  using FstImpl<A>::SetOutputSymbols;
  using FstImpl<A>::WriteHeader;

  using VectorFstBaseImpl<typename CacheImpl<A>::State>::NumStates;

  using CacheImpl<A>::HasArcs;
  using CacheImpl<A>::HasFinal;
  using CacheImpl<A>::HasStart;

  friend class StateIterator< CompactFst<A, C, U> >;

  typedef A Arc;
  typedef typename A::Weight Weight;
  typedef typename A::StateId StateId;
  typedef C Compactor;
  typedef typename C::Element CompactElement;
  typedef U Unsigned;

  CompactFstImpl()
      :  CacheImpl<A>(CompactFstOptions()),
         compactor_(0),
         own_compactor_(false),
         data_(0) {
    string type = "compact";
    if (sizeof(U) != sizeof(uint32)) {
      string size;
      Int64ToStr(8 * sizeof(U), &size);
      type += size;
    }
    type += "_";
    type += C::Type();
    SetType(type);
    SetProperties(kNullProperties | kStaticProperties);
  }

  CompactFstImpl(const Fst<Arc> &fst, const C &compactor,
                 const CompactFstOptions &opts)
      : CacheImpl<A>(opts),
        compactor_(new C(compactor)),
        own_compactor_(true),
        data_(0) {
    Init(fst);
  }

  CompactFstImpl(const Fst<Arc> &fst, C *compactor,
                 const CompactFstOptions &opts)
      : CacheImpl<A>(opts),
        compactor_(compactor),
        own_compactor_(false),
        data_(0) {
    Init(fst);
  }

  template <class Iterator>
  CompactFstImpl(const Iterator &b, const Iterator &e, const C &compactor,
                 const CompactFstOptions &opts)
      : CacheImpl<A>(opts),
        compactor_(new C(compactor)),
        own_compactor_(true),
        data_(0) {
    Init(b, e);
  }

  template <class Iterator>
  CompactFstImpl(const Iterator &b, const Iterator &e, C *compactor,
                 const CompactFstOptions &opts)
      : CacheImpl<A>(opts),
        compactor_(compactor),
        own_compactor_(false),
        data_(0) {
    Init(b, e);
  }

  CompactFstImpl(const CompactFstImpl<A, C, U> &impl)
      : CacheImpl<A>(impl),
        compactor_(new C(*impl.compactor_)),
        own_compactor_(true),
        data_(impl.data_) {
    if (data_)
      data_->IncrRefCount();
  }

  ~CompactFstImpl(){
    if (own_compactor_)
      delete compactor_;
    if (data_ && !data_->DecrRefCount())
      delete data_;
  }

  StateId Start() {
    if (!HasStart()) {
      SetStart(data_->Start());
    }
    return CacheImpl<A>::Start();
  }

  Weight Final(StateId s) {
    if (!HasFinal(s)) {
      Arc arc(kNoLabel, kNoLabel, Weight::Zero(), kNoStateId);
      if ((compactor_->Size() != -1) ||
          (data_->States(s) != data_->States(s + 1)))
        arc = compactor_->Expand(s, data_->Compacts(compactor_->Size() == -1
                                                    ? data_->States(s)
                                                    : s * compactor_->Size()));
      SetFinal(s, arc.ilabel == kNoLabel ? arc.weight : Weight::Zero());
    }
    return CacheImpl<A>::Final(s);
  }

  StateId NumStates() const { return data_->NumStates();}

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

  static CompactFstImpl<A, C, U> *Read(istream &strm,
                                       const FstReadOptions &opts) {
    CompactFstImpl<A, C, U> *impl = new CompactFstImpl<A, C, U>();
    FstHeader hdr;
    if (!impl->ReadHeader(strm, opts, kMinFileVersion, &hdr))
      return 0;

    impl->compactor_ = C::Read(strm);
    impl->own_compactor_ = true;
    impl->data_ = CompactFstData<A, C, U>::Read(strm, opts, hdr,
                                                *impl->compactor_);
    return impl;
  }

  bool Write(ostream &strm, const FstWriteOptions &opts) const {
    FstHeader hdr;
    hdr.SetStart(data_->Start());
    hdr.SetNumStates(data_->NumStates());
    hdr.SetNumArcs(data_->NumArcs());
    WriteHeader(strm, opts, kFileVersion, &hdr);
    compactor_->Write(strm);
    return data_->Write(strm, opts, *compactor_);
  }

  // Provide information needed for generic state iterator
  void InitStateIterator(StateIteratorData<A> *data) const {
    data->base = 0;
    data->nstates = data_->NumStates();
  }

  void InitArcIterator(StateId s, ArcIteratorData<A> *data) {
    if (!HasArcs(s))
      Expand(s);
    CacheImpl<A>::InitArcIterator(s, data);
  }

  void Expand(StateId s) {
    size_t begin =  compactor_->Size() == -1 ?
        data_->States(s) : s * compactor_->Size();
    size_t end = compactor_->Size() == -1 ?
        data_->States(s + 1) : (s + 1) * compactor_->Size();
    for (size_t i = begin; i < end; ++i) {
      const Arc &arc = compactor_->Expand(s, data_->Compacts(i));
      if (arc.ilabel == kNoLabel) continue;
      AddArc(s, arc);
    }
    SetArcs(s);
  }

 private:
  void Init(const Fst<Arc> &fst) {
    string type = "compact";
    if (sizeof(U) != sizeof(uint32)) {
      string size;
      Int64ToStr(8 * sizeof(U), &size);
      type += size;
    }
    type += "_";
    type += compactor_->Type();
    SetType(type);
    uint64 copy_properties = fst.Properties(kCopyProperties, true);
    if (!compactor_->Compatible(fst))
      LOG(FATAL) << "CompactFstImpl: input fst incompatible with compactor";
    SetProperties(copy_properties | kStaticProperties);
    SetInputSymbols(fst.InputSymbols());
    SetOutputSymbols(fst.OutputSymbols());
    data_ = new CompactFstData<A, C, U>(fst, *compactor_);
  }

  template <class Iterator>
  void Init(const Iterator &b, const Iterator &e) {
    string type = "compact_" + compactor_->Type();
    SetType(type);
    SetProperties(kStaticProperties | compactor_->Properties());
    data_ = new CompactFstData<A, C, U>(b, e, *compactor_);
  }

  // Properties always true of this Fst class
  static const uint64 kStaticProperties = kExpanded;
  // Current file format version
  static const int kFileVersion = 1;
  // Minimum file format version supported
  static const int kMinFileVersion = 1;

  C *compactor_;
  bool own_compactor_;
  CompactFstData<A, C, U> *data_;
};


// CompactFst.  This class attaches interface to implementation and
// handles reference counting. The unsigned type U is used to
// represent indices into the compact array (uint32 by default,
// declared in fst-decl.h).
template <class A, class C, class U>
class CompactFst : public ExpandedFst<A> {
 public:
  friend class StateIterator< CompactFst<A, C, U> >;
  friend class ArcIterator< CompactFst<A, C, U> >;
  friend class CacheArcIterator< CompactFst<A, C, U> >;
  template <class F, class G> void friend Cast(const F &, G *);

  typedef A Arc;
  typedef typename A::Weight Weight;
  typedef typename A::StateId StateId;
  typedef CacheState<A> State;
  typedef CompactFstImpl<A, C, U> Impl;
  typedef U Unsigned;

  CompactFst() : impl_(new CompactFstImpl<A, C, U>()) {}

  explicit CompactFst(const Fst<A> &fst, const C &compactor = C(),
                      const CompactFstOptions &opts = CompactFstOptions())
      : impl_(new CompactFstImpl<A, C, U>(fst, compactor, opts)) {}

  CompactFst(const Fst<A> &fst, C *compactor,
             const CompactFstOptions &opts = CompactFstOptions())
      : impl_(new CompactFstImpl<A, C, U>(fst, compactor, opts)) {}

  // The following 2 constructors take as input two iterators delimiting
  // a set of (already) compacted transitions, starting with the
  // transitions out of the initial state. The format of the input
  // differs for fixed out-degree and variable out-degree compactors.
  //
  // - For fixed out-degree compactors, the final weight (encoded as a
  // compacted transition) needs to be given only for final
  // states. All strings (compactor of size 1) will be assume to be
  // terminated by a final state even when the final state is not
  // implicitely given.
  //
  // - For variable out-degree compactors, the final weight (encoded
  // as a compacted transition) needs to be given for all states and
  // must appeared first in the list (for state s, final weight of s,
  // followed by outgoing transitons in s).
  //
  // These 2 constructors allows the direct construction of a CompactFst
  // without first creating a more memory hungry 'regular' FST. This
  // is useful when memory usage is severely constrained.
  template <class Iterator>
  explicit CompactFst(const Iterator &begin, const Iterator &end,
                      const C &compactor = C(),
                      const CompactFstOptions &opts = CompactFstOptions())
      : impl_(new CompactFstImpl<A, C, U>(begin, end, compactor, opts)) {}

  template <class Iterator>
  CompactFst(const Iterator &begin, const Iterator &end,
             C *compactor, const CompactFstOptions &opts = CompactFstOptions())
      : impl_(new CompactFstImpl<A, C, U>(begin, end, compactor, opts)) {}

  CompactFst(const CompactFst<A, C, U> &fst, bool reset = false) {
    if (reset) {
      impl_ = new CompactFstImpl<A, C, U>(*(fst.impl_));
    } else {
      impl_ = fst.impl_;
      impl_->IncrRefCount();
    }
  }

  virtual ~CompactFst() { if (!impl_->DecrRefCount()) delete impl_;  }

  virtual StateId Start() const { return impl_->Start(); }

  virtual Weight Final(StateId s) const { return impl_->Final(s); }

  StateId NumStates() const { return impl_->NumStates(); }

  size_t NumArcs(StateId s) const { return impl_->NumArcs(s); }

  size_t NumInputEpsilons(StateId s) const {
    return impl_->NumInputEpsilons(s);
  }

  size_t NumOutputEpsilons(StateId s) const {
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

  // Get a copy of this CompactFst
  virtual CompactFst<A, C, U> *Copy(bool reset = false) const {
    return new CompactFst<A, C, U>(*this, reset);
  }

  // Read a CompactFst from an input stream; return NULL on error
  static CompactFst<A, C, U> *Read(istream &strm, const FstReadOptions &opts) {
    CompactFstImpl<A, C, U>* impl = CompactFstImpl<A, C, U>::Read(strm, opts);
    return impl ? new CompactFst<A, C, U>(impl) : 0;
  }

  // Read a CompactFst from a file; return NULL on error
  // Empty filename reads from standard input
   static CompactFst<A, C, U> *Read(const string &filename) {

    if (!filename.empty()) {
      ifstream strm(filename.c_str());
      if (!strm) {
        LOG(ERROR) << "CompactFst::Write: Can't open file: " << filename;
        return 0;
      }
      return Read(strm, FstReadOptions(filename));
    } else {
      return Read(std::cin, FstReadOptions("standard input"));
    }
  }

  // Write a CompactFst to an output stream; return false on error
  virtual bool Write(ostream &strm, const FstWriteOptions &opts) const {
    return impl_->Write(strm, opts);
  }

  // Write a CompactFst to a file; return false on error
  // Empty filename writes to standard output
  virtual bool Write(const string &filename) const {
    if (!filename.empty()) {
      ofstream strm(filename.c_str());
      if (!strm) {
        LOG(ERROR) << "CompactFst::Write: Can't open file: " << filename;
        return false;
      }
      return Write(strm, FstWriteOptions(filename));
    } else {
      return Write(std::cout, FstWriteOptions("standard output"));
    }
  }

  virtual const SymbolTable* InputSymbols() const {
    return impl_->InputSymbols();
  }

  virtual const SymbolTable* OutputSymbols() const {
    return impl_->OutputSymbols();
  }

  virtual void InitStateIterator(StateIteratorData<A> *data) const {
    impl_->InitStateIterator(data);
  }

  virtual void InitArcIterator(StateId s, ArcIteratorData<A> *data) const {
    impl_->InitArcIterator(s, data);
  }

 private:
  CompactFst(CompactFstImpl<A, C, U> *impl) : impl_(impl) {}

  CompactFstImpl<A, C, U> *impl_;  // FST's impl

  void operator=(const CompactFst<A, C, U> &fst);  // disallow
};


// Specialization for CompactFst; see generic version in fst.h
// for sample usage (but use the CompactFst type!). This version
// should inline.
template <class A, class C, class U>
class StateIterator< CompactFst<A, C, U> > {
 public:
  typedef typename A::StateId StateId;

  explicit StateIterator(const CompactFst<A, C, U> &fst)
    : nstates_(fst.impl_->NumStates()), s_(0) {}

  bool Done() const { return s_ >= nstates_; }

  StateId Value() const { return s_; }

  void Next() { ++s_; }

  void Reset() { s_ = 0; }

 private:
  StateId nstates_;
  StateId s_;

  DISALLOW_COPY_AND_ASSIGN(StateIterator);
};

// Specialization for CompactFst.
template <class A, class C, class U>
class ArcIterator< CompactFst<A, C, U> >
    : public CacheArcIterator< CompactFst<A, C, U> > {
 public:
  typedef typename A::StateId StateId;

  ArcIterator(const CompactFst<A, C, U> &fst, StateId s)
      : CacheArcIterator< CompactFst<A, C, U> >(fst, s) {
    if (!fst.impl_->HasArcs(s))
      fst.impl_->Expand(s);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ArcIterator);
};


//
// Utility Compactors
//

// Compactor for unweighted string FSTs
template <class A>
class StringCompactor {
 public:
  typedef A Arc;
  typedef typename A::Label Element;
  typedef typename A::Label Label;
  typedef typename A::StateId StateId;
  typedef typename A::Weight Weight;

  Element Compact(StateId s, const A &arc) const { return arc.ilabel; }

  Arc Expand(StateId s, const Element &p) const {
    return Arc(p, p, Weight::One(), p != kNoLabel ? s + 1 : kNoStateId);
  }

  ssize_t Size() const { return 1; }

  uint64 Properties() const {
    return kString | kAcceptor | kUnweighted;
  }

  bool Compatible(const Fst<A> &fst) const {
    uint64 props = Properties();
    return fst.Properties(props, true) == props;
  }

  static const string &Type() {
    static const string type = "string";
    return type;
  }

  bool Write(ostream &strm) const { return true; }

  static StringCompactor *Read(istream &strm) {
    return new StringCompactor;
  }
};


// Compactor for weighted string FSTs
template <class A>
class WeightedStringCompactor {
 public:
  typedef A Arc;
  typedef typename A::Label Label;
  typedef typename A::StateId StateId;
  typedef typename A::Weight Weight;
  typedef pair<Label, Weight> Element;

  Element Compact(StateId s, const A &arc) const {
    return make_pair(arc.ilabel, arc.weight);
  }

  Arc Expand(StateId s, const Element &p) const {
    return Arc(p.first, p.first, p.second,
               p.first != kNoLabel ? s + 1 : kNoStateId);
  }

  ssize_t Size() const { return 1;}

  uint64 Properties() const {
    return kString | kAcceptor;
  }

  bool Compatible(const Fst<A> &fst) const {
    uint64 props = Properties();
    return fst.Properties(props, true) == props;
  }

  static const string &Type() {
    static const string type = "weighted_string";
    return type;
  }

  bool Write(ostream &strm) const { return true; }

  static WeightedStringCompactor *Read(istream &strm) {
    return new WeightedStringCompactor;
  }
};


// Compactor for unweighted acceptor FSTs
template <class A>
class UnweightedAcceptorCompactor {
 public:
  typedef A Arc;
  typedef typename A::Label Label;
  typedef typename A::StateId StateId;
  typedef typename A::Weight Weight;
  typedef pair<Label, StateId> Element;

  Element Compact(StateId s, const A &arc) const {
    return make_pair(arc.ilabel, arc.nextstate);
  }

  Arc Expand(StateId s, const Element &p) const {
    return Arc(p.first, p.first, Weight::One(), p.second);
  }

  ssize_t Size() const { return -1;}

  uint64 Properties() const {
    return kAcceptor | kUnweighted;
  }

  bool Compatible(const Fst<A> &fst) const {
    uint64 props = Properties();
    return fst.Properties(props, true) == props;
  }

  static const string &Type() {
    static const string type = "unweighted_acceptor";
    return type;
  }

  bool Write(ostream &strm) const { return true; }

  static UnweightedAcceptorCompactor *Read(istream &istrm) {
    return new UnweightedAcceptorCompactor;
  }
};


// Compactor for weighted acceptor FSTs
template <class A>
class AcceptorCompactor {
 public:
  typedef A Arc;
  typedef typename A::Label Label;
  typedef typename A::StateId StateId;
  typedef typename A::Weight Weight;
  typedef pair< pair<Label, Weight>, StateId > Element;

  Element Compact(StateId s, const A &arc) const {
    return make_pair(make_pair(arc.ilabel, arc.weight), arc.nextstate);
  }

  Arc Expand(StateId s, const Element &p) const {
    return Arc(p.first.first, p.first.first, p.first.second, p.second);
  }

  ssize_t Size() const { return -1;}

  uint64 Properties() const {
    return kAcceptor;
  }

  bool Compatible(const Fst<A> &fst) const {
    uint64 props = Properties();
    return fst.Properties(props, true) == props;
  }

  static const string &Type() {
    static const string type = "acceptor";
    return type;
  }

  bool Write(ostream &strm) const { return true; }

  static AcceptorCompactor *Read(istream &strm) {
    return new AcceptorCompactor;
  }
};


// Compactor for unweighted FSTs
template <class A>
class UnweightedCompactor {
 public:
  typedef A Arc;
  typedef typename A::Label Label;
  typedef typename A::StateId StateId;
  typedef typename A::Weight Weight;
  typedef pair< pair<Label, Label>, StateId > Element;

  Element Compact(StateId s, const A &arc) const {
    return make_pair(make_pair(arc.ilabel, arc.olabel), arc.nextstate);
  }

  Arc Expand(StateId s, const Element &p) const {
    return Arc(p.first.first, p.first.second, Weight::One(), p.second);
  }

  ssize_t Size() const { return -1; }

  uint64 Properties() const {
    return kUnweighted;
  }

  bool Compatible(const Fst<A> &fst) const {
    uint64 props = Properties();
    return fst.Properties(props, true) == props;
  }

  static const string &Type() {
    static const string type = "unweighted";
    return type;
  }

  bool Write(ostream &strm) const { return true; }

  static UnweightedCompactor *Read(istream &strm) {
    return new UnweightedCompactor;
  }
};


// Uselful aliases when using StdArc
typedef CompactFst< StdArc, StringCompactor<StdArc> >
StdCompactStringFst;
typedef CompactFst< StdArc, WeightedStringCompactor<StdArc> >
StdCompactWeightedStringFst;
typedef CompactFst<StdArc, AcceptorCompactor<StdArc> >
StdCompactAcceptorFst;
typedef CompactFst<StdArc, UnweightedCompactor<StdArc> >
StdCompactUnweightedFst;
typedef CompactFst<StdArc, UnweightedAcceptorCompactor<StdArc> >
StdCompactUnweightedAcceptorFst;

} // namespace fst

#endif // FST_LIB_COMPACT_FST_H__
