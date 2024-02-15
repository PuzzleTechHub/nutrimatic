// Copyright 2005-2024 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the 'License');
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an 'AS IS' BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#ifndef FST_TEST_COMPACTORS_H_
#define FST_TEST_COMPACTORS_H_

// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Compactors for use in tests.  See compact-fst.h.

#include <sys/types.h>

#include <cstddef>
#include <cstdint>
#include <istream>
#include <memory>
#include <ostream>
#include <string>
#include <type_traits>

#include <fst/arc.h>
#include <fst/expanded-fst.h>
#include <fst/fst.h>
#include <fst/properties.h>
#include <fst/util.h>
#include <fst/vector-fst.h>

namespace fst {

// A user-defined compactor for test FST.
// Stores all Arc components as a tuple.
template <class A>
class TrivialArcCompactor {
 public:
  using Arc = A;
  using Label = typename A::Label;
  using StateId = typename A::StateId;
  using Weight = typename A::Weight;
  // We use ArcTpl, which is trivially copyable if Weight is.
  static_assert(std::is_trivially_copyable_v<Weight>,
                "Weight must be trivially copyable.");
  using Element = ArcTpl<Weight>;
  static_assert(std::is_trivially_copyable_v<Element>,
                "ArcTpl should be trivially copyable; someone broke it.");

  Element Compact(StateId s, const A &arc) const {
    return Element(arc.ilabel, arc.olabel, arc.weight, arc.nextstate);
  }

  Arc Expand(StateId s, const Element &e, uint32_t f = kArcValueFlags) const {
    return Arc(e.ilabel, e.olabel, e.weight, e.nextstate);
  }

  ssize_t Size() const { return -1; }

  uint64_t Properties() const { return 0ULL; }

  bool Compatible(const Fst<A> &fst) const { return true; }

  static const std::string &Type() {
    static const std::string *const type =
        new std::string("trival_arc_compactor_" + Arc::Type());
    return *type;
  }

  bool Write(std::ostream &strm) const { return true; }

  static TrivialArcCompactor *Read(std::istream &strm) {
    return new TrivialArcCompactor;
  }
};

// A user-defined arc compactor for test FST.
// Doesn't actually do any compacting, but exercises the Compactor interface.
template <class A>
class TrivialCompactor {
 public:
  using Arc = A;
  using StateId = typename Arc::StateId;
  using Weight = typename Arc::Weight;

  // Any empty FST is OK.
  TrivialCompactor() : fst_(new VectorFst<Arc>) {}

  // Constructor from the Fst to be compacted.  If compactor is present,
  // only optional state should be copied from it.
  explicit TrivialCompactor(const Fst<Arc> &fst,
                            std::shared_ptr<TrivialCompactor> = nullptr)
      : fst_(fst.Copy()) {}

  // Copy constructor.  Must make a thread-safe copy suitable for use by
  // by Fst::Copy(/*safe=*/true).
  TrivialCompactor(const TrivialCompactor &compactor)
      : fst_(compactor.fst_->Copy(/*safe=*/true)) {}

  StateId Start() const { return fst_->Start(); }
  StateId NumStates() const { return CountStates(*fst_); }
  size_t NumArcs() const { return CountArcs(*fst_); }

  // Accessor class for state attributes.
  class State {
   public:
    State() = default;
    State(const TrivialCompactor *c, StateId s)
        : c_(c),
          s_(s),
          i_(std::make_unique<ArcIterator<Fst<Arc>>>(*c->fst_, s)) {}
    StateId GetStateId() const { return s_; }
    Weight Final() const { return c_->fst_->Final(s_); }
    size_t NumArcs() const { return c_->fst_->NumArcs(s_); }
    Arc GetArc(size_t i, uint32_t f) const {
      i_->Seek(i);
      return i_->Value();
    }

   private:
    const TrivialCompactor *c_ = nullptr;
    StateId s_ = kNoStateId;
    std::unique_ptr<ArcIterator<Fst<Arc>>> i_;
  };

  void SetState(StateId s, State *state) { *state = State(this, s); }

  template <typename Arc>
  bool IsCompatible(const Fst<Arc> &fst) const {
    return std::is_same_v<Arc, A>;
  }

  uint64_t Properties(uint64_t props) const { return props; }

  static const std::string &Type() {
    static const std::string *const type =
        new std::string("trivial_compactor_" + Arc::Type());
    return *type;
  }

  bool Error() const { return fst_->Properties(kError, /*test=*/false); }

  bool Write(std::ostream &strm, const FstWriteOptions &opts) const {
    WriteType(strm, Type());
    // Write as a VectorFst.
    return VectorFst<Arc>::WriteFst(*fst_, strm, opts);
  }

  static TrivialCompactor *Read(std::istream &strm, FstReadOptions opts,
                                const FstHeader &hdr) {
    std::string type;
    ReadType(strm, &type);
    if (type != Type()) return nullptr;
    opts.header = nullptr;
    auto fst = fst::WrapUnique(VectorFst<Arc>::Read(strm, opts));
    if (fst == nullptr) return nullptr;
    return new TrivialCompactor(*fst);
  }

 private:
  std::unique_ptr<Fst<Arc>> fst_;
};

}  // namespace fst

#endif  // FST_TEST_COMPACTORS_H_
