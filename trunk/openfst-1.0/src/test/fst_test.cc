// fst_test.cc

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
// Regression test for FST classes.

#include <fst/compact-fst.h>
#include <fst/const-fst.h>
#include <fst/equal.h>
#include <fst/vector-fst.h>
#include <fst/verify.h>

namespace fst {

// This tests an Fst F that is assumed to have a copy method from an
// arbitrary Fst. Some test functions make further assumptions mostly
// obvious from their name. These tests are written as member temple
// functions that take a test fst as its argument so that different
// Fsts in the interface hierarchy can be tested separately and so
// that we can instantiate only those tests that make sense for a
// particular Fst.
template <class F>
class FstTester {
 public:
  typedef typename F::Arc Arc;
  typedef typename Arc::StateId StateId;
  typedef typename Arc::Weight Weight;
  typedef typename Arc::Label Label;

  FstTester() {
    VectorFst<Arc> vfst;
    InitFst(&vfst, 128);
    testfst_ = new F(vfst);
  }

  ~FstTester() {
    delete testfst_;
  }

  // This verifies the contents described in InitFst() using
  // methods defined in a generic Fst.
  template <class G>
  void TestBase(const G &fst) const {
    CHECK(Verify(fst));
    CHECK(fst.Start() == 0);
    StateId ns = 0;
    StateIterator<G> siter(fst);
    for (; !siter.Done(); siter.Next());
    for (siter.Reset(); !siter.Done(); siter.Next()) {
      StateId s = siter.Value();
      CHECK(fst.Final(s) == NthWeight(s));
      size_t na = 0;
      ArcIterator<G> aiter(fst, s);
      for (; !aiter.Done(); aiter.Next());
      for (aiter.Reset(); !aiter.Done(); aiter.Next()) {
        ++na;
        const Arc &arc = aiter.Value();
        CHECK(arc.ilabel == na);
        CHECK(arc.olabel == 0);
        CHECK(arc.weight == NthWeight(na));
        CHECK(arc.nextstate == s);
      }
      CHECK(na == s);
      CHECK(na == aiter.Position());
      CHECK(fst.NumArcs(s) == s);
      CHECK(fst.NumInputEpsilons(s) ==  0);
      CHECK(fst.NumOutputEpsilons(s) == s);
      CHECK(fst.Properties(kNotAcceptor, true));
      CHECK(fst.Properties(kOEpsilons, true));
      ++ns;
    }
  }

  void TestBase() const { TestBase(*testfst_); }

  // This verifies methods specfic to an ExpandedFst.
  template <class G>
  void TestExpanded(const G &fst) const {
    StateId ns = 0;
    for (StateIterator<G> siter(fst);
         !siter.Done();
         siter.Next()) {
      ++ns;
    }
    CHECK(fst.NumStates() == ns);
    CHECK(fst.Properties(kExpanded, false));
  }

  void TestExpanded() const { TestExpanded(*testfst_); }

  // This verifies methods specfic to a MutableFst.
  template <class G>
  void TestMutable(G *fst) const {
    for (StateIterator<G> siter(*fst);
         !siter.Done();
         siter.Next()) {
      StateId s = siter.Value();
      size_t na = 0;
      size_t ni = fst->NumInputEpsilons(s);
      MutableArcIterator<G> aiter(fst, s);
      for (; !aiter.Done(); aiter.Next());
      for (aiter.Reset(); !aiter.Done(); aiter.Next()) {
        ++na;
        Arc arc = aiter.Value();
        arc.ilabel = 0;
        aiter.SetValue(arc);
        arc = aiter.Value();
        CHECK(arc.ilabel == 0);
        CHECK(fst->NumInputEpsilons(s) == ni + 1);
        arc.ilabel = na;
        aiter.SetValue(arc);
        CHECK(fst->NumInputEpsilons(s) == ni);
      }
    }

    G *cfst1 = fst->Copy();
    cfst1->DeleteStates();
    CHECK(cfst1->NumStates() == 0);
    delete cfst1;

    G *cfst2 = fst->Copy();
    for (StateIterator<G> siter(*cfst2);
         !siter.Done();
         siter.Next()) {
      StateId s = siter.Value();
      cfst2->DeleteArcs(s);
      CHECK(cfst2->NumArcs(s) == 0);
      CHECK(cfst2->NumInputEpsilons(s) == 0);
      CHECK(cfst2->NumOutputEpsilons(s) == 0);
    }
    delete cfst2;
  }

  void TestMutable() { TestMutable(testfst_); }

  // This verifies the copy methods.
  template <class G>
  void TestAssign(G *fst) const {
    // Assignment from G
    G afst1;
    afst1 = *fst;
    CHECK(Equal(*fst, afst1));

    // Assignment from Fst
    G afst2;
    afst2 = *static_cast<const Fst<Arc> *>(fst);
    CHECK(Equal(*fst, afst2));

    // Assignment from self
    afst2 = afst2;
    CHECK(Equal(*fst, afst2));
  }

  void TestAssign() { TestAssign(testfst_); }

  // This verifies the copy methods.
  template <class G>
  void TestCopy(const G &fst) const {
    // Copy from G
    G c1fst(fst);
    TestBase(c1fst);

    // Copy from Fst
    const G c2fst(static_cast<const Fst<Arc> &>(fst));
    TestBase(c2fst);

    // Copy from self
    const G *c3fst = fst.Copy();
    TestBase(*c3fst);
    delete c3fst;
  }

  void TestCopy() const { TestCopy(*testfst_); }

  // This verifies the read/write methods.
  template <class G>
  void TestIO(const G &fst) const {
    const string filename = FLAGS_tmpdir + "/test.fst";
    {
      // write/read
      CHECK(fst.Write(filename));
      G *ffst = G::Read(filename);
      CHECK(ffst);
      TestBase(*ffst);
      delete ffst;
    }

    {
      // generic read/cast/test
      Fst<Arc> *gfst = Fst<Arc>::Read(filename);
      CHECK(gfst);
      G *dfst = down_cast<G *>(gfst);
      TestBase(*dfst);

      // generic write/read/test
      CHECK(gfst->Write(filename));
      Fst<Arc> *hfst = Fst<Arc>::Read(filename);
      CHECK(hfst);
      TestBase(*hfst);
      delete gfst;
      delete hfst;
    }

    // expanded write/read/test
    if (fst.Properties(kExpanded, false)) {
      ExpandedFst<Arc> *efst = ExpandedFst<Arc>::Read(filename);
      CHECK(efst);
      TestBase(*efst);
      TestExpanded(*efst);
      delete efst;
    }

    // mutable write/read/test
    if (fst.Properties(kMutable, false)) {
      MutableFst<Arc> *mfst = MutableFst<Arc>::Read(filename);
      CHECK(mfst);
      TestBase(*mfst);
      TestExpanded(*mfst);
      TestMutable(mfst);
      delete mfst;
    }
  }

  void TestIO() const { TestIO(*testfst_); }

 private:
  // This constructs test FSTs. Given a mutable FST, will leave
  // the FST as follows:
  // (I) NumStates() = nstates
  // (II) Start() = 0
  // (III) Final(s) =  NthWeight(s)
  // (IV) For state s:
  //     (a) NumArcs(s) == s
  //     (b) For ith arc of s:
  //         (1) ilabel = i
  //         (2) olabel = 0
  //         (3) weight = NthWeight(i)
  //         (4) nextstate = s
  void InitFst(MutableFst<Arc> *fst, size_t nstates) const {
    fst->DeleteStates();
    CHECK(nstates > 0);

    for (StateId s = 0; s < nstates; ++s) {
      fst->AddState();
      fst->SetFinal(s, NthWeight(s));
      for (size_t i = 1; i <= s; ++i) {
        Arc arc(i, 0, NthWeight(i), s);
        fst->AddArc(s, arc);
      }
    }

    fst->SetStart(0);
  }

  // Generates One() + ... + One() (n times)
  Weight NthWeight(int n) const {
    Weight w = Weight::Zero();
    for (int i = 0; i < n; ++i)
      w = Plus(w, Weight::One());
    return w;
  }

 private:
  F *testfst_;   // what we're testing
};

// A user-defined arc type.
struct CustomArc {
  typedef short Label;
  typedef ProductWeight<TropicalWeight, LogWeight> Weight;
  typedef int64 StateId;

  CustomArc(Label i, Label o, Weight w, StateId s) :
    ilabel(i), olabel(o), weight(w), nextstate(s) {}
  CustomArc() {}

  static const string &Type() {  // Arc type name
    static const string type = "my";
    return type;
  }

  Label ilabel;       // Transition input label
  Label olabel;       // Transition output label
  Weight weight;      // Transition weight
  StateId nextstate;  // Transition destination state
};


// A user-defined compactor for test FST.
template <class A>
class CustomCompactor {
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
    return p.first == kNoLabel ?
        Arc(kNoLabel, kNoLabel, p.second, kNoStateId) :
        Arc(p.first, 0, p.second, s);
  }

  ssize_t Size() const { return -1;}

  uint64 Properties() const { return 0ULL;}

  bool Compatible(const Fst<A> &fst) const {
    return true;
  }

  static const string &Type() {
    static const string type = "my";
    return type;
  }

  bool Write(ostream &strm) const { return true; }

  static CustomCompactor *Read(istream &strm) {
    return new CustomCompactor;
  }
};


REGISTER_FST(VectorFst, CustomArc);
REGISTER_FST(ConstFst, CustomArc);
static fst::FstRegisterer<
  CompactFst<StdArc, CustomCompactor<StdArc> > >
CompactFst_StdArc_CustomCompactor_registerer;
static fst::FstRegisterer<
  CompactFst<CustomArc, CustomCompactor<CustomArc> > >
CompactFst_CustomArc_CustomCompactor_registerer;
static fst::FstRegisterer<ConstFst<StdArc, uint16> >
ConstFst_StdArc_uint16_registerer;
static fst::FstRegisterer<
  CompactFst<StdArc, CustomCompactor<StdArc>, uint16> >
CompactFst_StdArc_CustomCompactor_uint16_registerer;

}  // namespace fst


using fst::FstTester;
using fst::VectorFst;
using fst::ConstFst;
using fst::CompactFst;
using fst::StdArc;
using fst::CustomArc;
using fst::CustomCompactor;

int main(int argc, char **argv) {
  FLAGS_fst_verify_properties = true;
  std::set_new_handler(FailedNewHandler);
  SetFlags(argv[0], &argc, &argv, true);

  // VectorFst<StdArc> tests
  {
    FstTester< VectorFst<StdArc> > std_vector_tester;
    std_vector_tester.TestBase();
    std_vector_tester.TestExpanded();
    std_vector_tester.TestAssign();
    std_vector_tester.TestCopy();
    std_vector_tester.TestIO();
    std_vector_tester.TestMutable();
  }

  // ConstFst<StdArc> tests
  {
    FstTester< ConstFst<StdArc> > std_const_tester;
    std_const_tester.TestBase();
    std_const_tester.TestExpanded();
    std_const_tester.TestCopy();
    std_const_tester.TestIO();
  }

  // CompactFst<StdArc, CustomCompactor<StdArc> >
  {
    FstTester< CompactFst<StdArc, CustomCompactor<StdArc> > >
        std_compact_tester;
    std_compact_tester.TestBase();
    std_compact_tester.TestExpanded();
    std_compact_tester.TestCopy();
    std_compact_tester.TestIO();
  }

  // VectorFst<CustomArc> tests
  {
    FstTester< VectorFst<CustomArc> > std_vector_tester;
    std_vector_tester.TestBase();
    std_vector_tester.TestExpanded();
    std_vector_tester.TestAssign();
    std_vector_tester.TestCopy();
    std_vector_tester.TestIO();
    std_vector_tester.TestMutable();
  }

  // ConstFst<CustomArc> tests
  {
    FstTester< ConstFst<CustomArc> > std_const_tester;
    std_const_tester.TestBase();
    std_const_tester.TestExpanded();
    std_const_tester.TestCopy();
    std_const_tester.TestIO();
  }

  // CompactFst<CustomArc, CustomCompactor<CustomArc> >
  {
    FstTester< CompactFst<CustomArc, CustomCompactor<CustomArc> > >
        std_compact_tester;
    std_compact_tester.TestBase();
    std_compact_tester.TestExpanded();
    std_compact_tester.TestCopy();
    std_compact_tester.TestIO();
  }

  // ConstFst<StdArc, uint16> tests
  {
    FstTester< ConstFst<StdArc, uint16> > std_const_tester;
    std_const_tester.TestBase();
    std_const_tester.TestExpanded();
    std_const_tester.TestCopy();
    std_const_tester.TestIO();
  }

  // CompactFst<StdArc, CustomCompactor<StdArc>, uint16>
  {
    FstTester< CompactFst<StdArc, CustomCompactor<StdArc>, uint16> >
        std_compact_tester;
    std_compact_tester.TestBase();
    std_compact_tester.TestExpanded();
    std_compact_tester.TestCopy();
    std_compact_tester.TestIO();
  }

  cout << "PASS" << endl;

  return 0;
}
