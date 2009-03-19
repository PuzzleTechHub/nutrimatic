// weight_test.h

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
// Regression test for various FST algorithms.

#include <fst/fstlib.h>
#include <fst/random-weight.h>

// These determine which semirings are tested. Defining at least
// TEST_TROPICAL and TEST_LOG is recommended. More increase the
// comprehensiveness, but also increase the compilation time.

#define TEST_TROPICAL
#define TEST_LOG
// #define TEST_MINMAX
// #define TEST_LEFT_STRING
// #define TEST_RIGHT_STRING
// #define TEST_GALLIC
// #define TEST_LEXICOGRAPHIC

DEFINE_int32(seed, -1, "random seed");
DEFINE_int32(repeat, 25, "number of test repetitions");

namespace fst {

// Mapper to change input and output label of every transition into
// epsilons.
template <class A>
class EpsMapper {
 public:
  EpsMapper() {}

  A operator()(const A &arc) const {
    return A(0, 0, arc.weight, arc.nextstate);
  }

  uint64 Properties(uint64 props) const {
    props &= ~kNotAcceptor;
    props |= kAcceptor;
    props &= ~kNoIEpsilons & ~kNoOEpsilons &  ~kNoEpsilons;
    props |= kIEpsilons | kOEpsilons | kEpsilons;
    props &= ~kNotILabelSorted & ~kNotOLabelSorted;
    props |= kILabelSorted | kOLabelSorted;
    return props;
  }

  MapFinalAction FinalAction() const { return MAP_NO_SUPERFINAL; }


  MapSymbolsAction InputSymbolsAction() const { return MAP_COPY_SYMBOLS;}

  MapSymbolsAction OutputSymbolsAction() const { return MAP_COPY_SYMBOLS;}
};

// This class tests a variety of identities and properties that must
// hold for various algorithms on weighted FSTs.
template <class Arc, class WeightGenerator>
class WeightedTester {
 public:
  typedef typename Arc::Label Label;
  typedef typename Arc::StateId StateId;
  typedef typename Arc::Weight Weight;

  WeightedTester(int seed, const Fst<Arc> &zero_fst, const Fst<Arc> &one_fst,
                 const Fst<Arc> &univ_fst, WeightGenerator *weight_generator)
      : seed_(seed), zero_fst_(zero_fst), one_fst_(one_fst),
        univ_fst_(univ_fst), weight_generator_(weight_generator) {}

  void Test(const Fst<Arc> &T1, const Fst<Arc> &T2, const Fst<Arc> &T3) {
    TestRational(T1, T2, T3);
    TestMap(T1);
    TestCompose(T1, T2, T3);
    TestSort(T1);
    TestOptimize(T1);
    TestSearch(T1);
  }

 private:

  // Tests rational operations with identities
  void TestRational(const Fst<Arc> &T1, const Fst<Arc> &T2,
                    const Fst<Arc> &T3) {

    // Checks destructive and delayed union are equivalent.
    {
      VectorFst<Arc> U1(T1);
      Union(&U1,  T2);
      UnionFst<Arc> U2(T1, T2);
      CHECK(Equiv(U1, U2));
    }

    // Checks destructive and delayed concatenation are equivalent.
    {
      VectorFst<Arc> C1(T1);
      Concat(&C1,  T2);
      ConcatFst<Arc> C2(T1, T2);
      CHECK(Equiv(C1, C2));
      VectorFst<Arc> C3(T2);
      Concat(T1, &C3);
      CHECK(Equiv(C3, C2));
    }

    // Checks destructive and delayed closure* are equivalent.
    {
      VectorFst<Arc> C1(T1);
      Closure(&C1, CLOSURE_STAR);
      ClosureFst<Arc> C2(T1, CLOSURE_STAR);
      CHECK(Equiv(C1, C2));
    }

    // Checks destructive and delayed closure+ are equivalent.
    {
      VectorFst<Arc> C1(T1);
      Closure(&C1, CLOSURE_PLUS);
      ClosureFst<Arc> C2(T1, CLOSURE_PLUS);
      CHECK(Equiv(C1, C2));
    }

    // Checks union is associative (destructive).
    {
      VectorFst<Arc> U1(T1);
      Union(&U1, T2);
      Union(&U1, T3);

      VectorFst<Arc> U3(T2);
      Union(&U3, T3);
      VectorFst<Arc> U4(T1);
      Union(&U4, U3);

      CHECK(Equiv(U1, U4));
    }

    // Checks union is associative (delayed).
    {
      UnionFst<Arc> U1(T1, T2);
      UnionFst<Arc> U2(U1, T3);

      UnionFst<Arc> U3(T2, T3);
      UnionFst<Arc> U4(T1, U3);

      CHECK(Equiv(U2, U4));
    }

    // Checks union is associative (destructive delayed).
    {
      UnionFst<Arc> U1(T1, T2);
      Union(&U1, T3);

      UnionFst<Arc> U3(T2, T3);
      UnionFst<Arc> U4(T1, U3);

      CHECK(Equiv(U1, U4));
    }

    // Checks concatenation is associative (destructive).
    {
      VectorFst<Arc> C1(T1);
      Concat(&C1, T2);
      Concat(&C1, T3);

      VectorFst<Arc> C3(T2);
      Concat(&C3, T3);
      VectorFst<Arc> C4(T1);
      Concat(&C4, C3);

      CHECK(Equiv(C1, C4));
    }

    // Checks concatenation is associative (delayed).
    {
      ConcatFst<Arc> C1(T1, T2);
      ConcatFst<Arc> C2(C1, T3);

      ConcatFst<Arc> C3(T2, T3);
      ConcatFst<Arc> C4(T1, C3);

      CHECK(Equiv(C2, C4));
    }

    // Checks concatenation is associative (destructive delayed).
    {
      ConcatFst<Arc> C1(T1, T2);
      Concat(&C1, T3);

      ConcatFst<Arc> C3(T2, T3);
      ConcatFst<Arc> C4(T1, C3);

      CHECK(Equiv(C1, C4));
    }

    // Checks concatenation left distributes over union (destructive).
    if (Weight::Properties() & kLeftSemiring) {
      VectorFst<Arc> U1(T1);
      Union(&U1, T2);
      VectorFst<Arc> C1(T3);
      Concat(&C1, U1);

      VectorFst<Arc> C2(T3);
      Concat(&C2, T1);
      VectorFst<Arc> C3(T3);
      Concat(&C3, T2);
      VectorFst<Arc> U2(C2);
      Union(&U2, C3);

      CHECK(Equiv(C1, U2));
    }

    // Checks concatenation right distributes over union (destructive).
    if (Weight::Properties() & kRightSemiring) {
      VectorFst<Arc> U1(T1);
      Union(&U1, T2);
      VectorFst<Arc> C1(U1);
      Concat(&C1, T3);

      VectorFst<Arc> C2(T1);
      Concat(&C2, T3);
      VectorFst<Arc> C3(T2);
      Concat(&C3, T3);
      VectorFst<Arc> U2(C2);
      Union(&U2, C3);

      CHECK(Equiv(C1, U2));
    }

    // Checks concatenation left distributes over union (delayed).
    if (Weight::Properties() & kLeftSemiring) {
      UnionFst<Arc> U1(T1, T2);
      ConcatFst<Arc> C1(T3, U1);

      ConcatFst<Arc> C2(T3, T1);
      ConcatFst<Arc> C3(T3, T2);
      UnionFst<Arc> U2(C2, C3);

      CHECK(Equiv(C1, U2));
    }

    // Checks concatenation right distributes over union (delayed).
    if (Weight::Properties() & kRightSemiring) {
      UnionFst<Arc> U1(T1, T2);
      ConcatFst<Arc> C1(U1, T3);

      ConcatFst<Arc> C2(T1, T3);
      ConcatFst<Arc> C3(T2, T3);
      UnionFst<Arc> U2(C2, C3);

      CHECK(Equiv(C1, U2));
    }

    // Checks T T* == T+ (destructive).
    if (Weight::Properties() & kLeftSemiring) {
      VectorFst<Arc> S(T1);
      Closure(&S, CLOSURE_STAR);
      VectorFst<Arc> C(T1);
      Concat(&C, S);

      VectorFst<Arc> P(T1);
      Closure(&P, CLOSURE_PLUS);

      CHECK(Equiv(C, P));
    }

    // Checks T* T == T+ (destructive).
    if (Weight::Properties() & kRightSemiring) {
      VectorFst<Arc> S(T1);
      Closure(&S, CLOSURE_STAR);
      VectorFst<Arc> C(S);
      Concat(&C, T1);

      VectorFst<Arc> P(T1);
      Closure(&P, CLOSURE_PLUS);

      CHECK(Equiv(C, P));
   }

    // Checks T T* == T+ (delayed).
    if (Weight::Properties() & kLeftSemiring) {
      ClosureFst<Arc> S(T1, CLOSURE_STAR);
      ConcatFst<Arc> C(T1, S);

      ClosureFst<Arc> P(T1, CLOSURE_PLUS);

      CHECK(Equiv(C, P));
    }

    // Checks T* T == T+ (delayed).
    if (Weight::Properties() & kRightSemiring) {
      ClosureFst<Arc> S(T1, CLOSURE_STAR);
      ConcatFst<Arc> C(S, T1);

      ClosureFst<Arc> P(T1, CLOSURE_PLUS);

      CHECK(Equiv(C, P));
    }
  }

  // Tests map-based operations.
  void TestMap(const Fst<Arc> &T) {

    // Checks destructive and delayed projection are equivalent.
    {
      VectorFst<Arc> P1(T);
      Project(&P1, PROJECT_INPUT);
      ProjectFst<Arc> P2(T, PROJECT_INPUT);
      CHECK(Equiv(P1, P2));
    }

    // Checks destructive and delayed inversion are equivalent.
    {
      VectorFst<Arc> I1(T);
      Invert(&I1);
      InvertFst<Arc> I2(T);
      CHECK(Equiv(I1, I2));
    }

    // Checks Pi_1(T) = Pi_2(T^-1) (destructive).
    {
      VectorFst<Arc> P1(T);
      VectorFst<Arc> I1(T);
      Project(&P1, PROJECT_INPUT);
      Invert(&I1);
      Project(&I1, PROJECT_OUTPUT);
      CHECK(Equiv(P1, I1));
    }

    // Checks Pi_2(T) = Pi_1(T^-1) (destructive).
    {
      VectorFst<Arc> P1(T);
      VectorFst<Arc> I1(T);
      Project(&P1, PROJECT_OUTPUT);
      Invert(&I1);
      Project(&I1, PROJECT_INPUT);
      CHECK(Equiv(P1, I1));
    }

    // Checks Pi_1(T) = Pi_2(T^-1) (delayed).
    {
      ProjectFst<Arc> P1(T, PROJECT_INPUT);
      InvertFst<Arc> I1(T);
      ProjectFst<Arc> P2(I1, PROJECT_OUTPUT);
      CHECK(Equiv(P1, P2));
    }

    // Checks Pi_2(T) = Pi_1(T^-1) (delayed).
    {
      ProjectFst<Arc> P1(T, PROJECT_OUTPUT);
      InvertFst<Arc> I1(T);
      ProjectFst<Arc> P2(I1, PROJECT_INPUT);
      CHECK(Equiv(P1, P2));
    }

    // Checks destructive relabeling
    {
      static const int kNumLabels = 10;
      // set up relabeling pairs
      vector<Label> labelset(kNumLabels);
      for (size_t i = 0; i < kNumLabels; ++i) labelset[i] = i;
      for (size_t i = 0; i < kNumLabels; ++i) {
        swap(labelset[i], labelset[rand() % kNumLabels]);
      }

      vector<pair<Label, Label> > ipairs1(kNumLabels);
      vector<pair<Label, Label> > opairs1(kNumLabels);
      for (size_t i = 0; i < kNumLabels; ++i) {
        ipairs1[i] = make_pair(i, labelset[i]);
        opairs1[i] = make_pair(labelset[i], i);
      }
      VectorFst<Arc> R(T);
      Relabel(&R, ipairs1, opairs1);

      vector<pair<Label, Label> > ipairs2(kNumLabels);
      vector<pair<Label, Label> > opairs2(kNumLabels);
      for (size_t i = 0; i < kNumLabels; ++i) {
        ipairs2[i] = make_pair(labelset[i], i);
        opairs2[i] = make_pair(i, labelset[i]);
      }
      Relabel(&R, ipairs2, opairs2);
      CHECK(Equiv(R, T));

      // Checks on-the-fly relabeling
      RelabelFst<Arc> Rdelay(T, ipairs1, opairs1);

      RelabelFst<Arc> RRdelay(Rdelay, ipairs2, opairs2);
      CHECK(Equiv(RRdelay, T));
    }

    // Checks encoding/decoding (destructive).
    {
      VectorFst<Arc> D(T);
      uint32 encode_props = 0;
      if (rand() % 2)
        encode_props |= kEncodeLabels;
      if (rand() % 2)
        encode_props |= kEncodeWeights;
      EncodeMapper<Arc> encoder(encode_props, ENCODE);
      Encode(&D, &encoder);
      Decode(&D, encoder);
      CHECK(Equiv(D, T));
    }

    // Checks encoding/decoding (delayed).
    {
      uint32 encode_props = 0;
      if (rand() % 2)
        encode_props |= kEncodeLabels;
      if (rand() % 2)
        encode_props |= kEncodeWeights;
      EncodeMapper<Arc> encoder(encode_props, ENCODE);
      EncodeFst<Arc> E(T, &encoder);
      VectorFst<Arc> Encoded(E);
      DecodeFst<Arc> D(Encoded, encoder);
      CHECK(Equiv(D, T));
    }

    // Checks gallic mappers (constructive).
    {
      ToGallicMapper<Arc> to_mapper;
      FromGallicMapper<Arc> from_mapper;
      VectorFst< GallicArc<Arc> > G;
      VectorFst<Arc> F;
      Map(T, &G, to_mapper);
      Map(G, &F, from_mapper);
      CHECK(Equiv(T, F));
    }

    // Checks gallic mappers (delayed).
    {
      ToGallicMapper<Arc> to_mapper;
      FromGallicMapper<Arc> from_mapper;
      MapFst<Arc, GallicArc<Arc>, ToGallicMapper<Arc> >
        G(T, to_mapper);
      MapFst<GallicArc<Arc>, Arc, FromGallicMapper<Arc> >
        F(G, from_mapper);
      CHECK(Equiv(T, F));
    }
  }

  // Tests compose-based operations.
  void TestCompose(const Fst<Arc> &T1, const Fst<Arc> &T2,
                   const Fst<Arc> &T3) {
    if (!(Weight::Properties() & kCommutative))
      return;

    VectorFst<Arc> S1(T1);
    VectorFst<Arc> S2(T2);
    VectorFst<Arc> S3(T3);

    ILabelCompare<Arc> icomp;
    OLabelCompare<Arc> ocomp;

    ArcSort(&S1, ocomp);
    ArcSort(&S2, ocomp);
    ArcSort(&S3, icomp);


    // Checks composition is associative.
    {
      ComposeFst<Arc> C1(S1, S2);
      ComposeFst<Arc> C2(C1, S3);

      ComposeFst<Arc> C3(S2, S3);
      ComposeFst<Arc> C4(S1, C3);

      CHECK(Equiv(C2, C4));
    }

    // Checks composition left distributes over union.
    {
      UnionFst<Arc> U1(S2, S3);
      ComposeFst<Arc> C1(S1, U1);

      ComposeFst<Arc> C2(S1, S2);
      ComposeFst<Arc> C3(S1, S3);
      UnionFst<Arc> U2(C2, C3);

      CHECK(Equiv(C1, U2));
    }

    // Checks composition right distributes over union.
    {
      UnionFst<Arc> U1(S1, S2);
      ComposeFst<Arc> C1(U1, S3);

      ComposeFst<Arc> C2(S1, S3);
      ComposeFst<Arc> C3(S2, S3);
      UnionFst<Arc> U2(C2, C3);

      CHECK(Equiv(C1, U2));
    }

    VectorFst<Arc> A1(S1);
    VectorFst<Arc> A2(S2);
    VectorFst<Arc> A3(S3);
    Project(&A1, PROJECT_OUTPUT);
    Project(&A2, PROJECT_INPUT);
    Project(&A3, PROJECT_INPUT);

    // Checks intersection is commutative.
    {
      IntersectFst<Arc> I1(A1, A2);
      IntersectFst<Arc> I2(A2, A1);
      CHECK(Equiv(I1, I2));
    }

    // Checks all filters leads to equivalent results.
    {
      ComposeFst<Arc> C1(S1, S2);
      ComposeFst<Arc> C2(
          S1, S2,
          ComposeFstOptions<Arc, Matcher<Fst<Arc> >,
          AltSequenceComposeFilter<Arc> >());
      ComposeFst<Arc> C3(
          S1, S2,
          ComposeFstOptions<Arc, Matcher<Fst<Arc> >,
          MatchComposeFilter<Arc> >());

      CHECK(Equiv(C1, C2));
      CHECK(Equiv(C1, C3));
    }
  }

  // Tests sorting operations
  void TestSort(const Fst<Arc> &T) {
    ILabelCompare<Arc> icomp;
    OLabelCompare<Arc> ocomp;

    // Checks arc sorted Fst is equivalent to its input.
    {
      VectorFst<Arc> S1(T);
      ArcSort(&S1, icomp);
      CHECK(Equiv(T, S1));
    }

    // Checks destructive and delayed arcsort are equivalent.
    {
      VectorFst<Arc> S1(T);
      ArcSort(&S1, icomp);
      ArcSortFst< Arc, ILabelCompare<Arc> > S2(T, icomp);
      CHECK(Equiv(S1, S2));
    }

    // Checks ilabel sorting vs. olabel sorting with inversions.
    {
      VectorFst<Arc> S1(T);
      VectorFst<Arc> S2(T);
      ArcSort(&S1, icomp);
      Invert(&S2);
      ArcSort(&S2, ocomp);
      Invert(&S2);
      CHECK(Equiv(S1, S2));
    }

    // Checks topologically sorted Fst is equivalent to its input.
    {
      VectorFst<Arc> S1(T);
      TopSort(&S1);
      CHECK(Equiv(T, S1));
    }

    // Checks reverse(reverse(T)) = T
    {
      VectorFst< ReverseArc<Arc> > R1;
      VectorFst<Arc> R2;
      Reverse(T, &R1);
      Reverse(R1, &R2);
      CHECK(Equiv(T, R2));
    }
  }

  // Tests optimization operations
  void TestOptimize(const Fst<Arc> &T) {
    uint64 tprops = T.Properties(kFstProperties, true);
    uint64 wprops = Weight::Properties();

    VectorFst<Arc> A(T);
    Project(&A, PROJECT_INPUT);

    // Checks connected FST is equivalent to its input.
    {
      VectorFst<Arc> C1(T);
      Connect(&C1);
      CHECK(Equiv(T, C1));
    }

    if ((wprops & kSemiring) == kSemiring &&
        (tprops & kAcyclic || wprops & kIdempotent)) {
      // Checks epsilon-removed FST is equivalent to its input.
      VectorFst<Arc> R1(T);
      RmEpsilon(&R1);
      CHECK(Equiv(T, R1));

      // Checks destructive and delayed epsilon removal are equivalent.
      RmEpsilonFst<Arc> R2(T);
      CHECK(Equiv(R1, R2));

      // Checks an FST with a large proportion of epsilon transitions:
      // Maps all transitions of T to epsilon-transitions and append
      // a non-epsilon transition.
      VectorFst<Arc> U;
      Map(T, &U, EpsMapper<Arc>());
      VectorFst<Arc> V;
      V.SetStart(V.AddState());
      Arc arc(1, 1, Weight::One(), V.AddState());
      V.AddArc(V.Start(), arc);
      V.SetFinal(arc.nextstate, Weight::One());
      Concat(&U, V);
      // Check that epsilon-removal preserves the shortest-distance
      // from the initial state to the final states.
      vector<Weight> d;
      ShortestDistance(U, &d, true);
      Weight w = U.Start() < d.size() ? d[U.Start()] : Weight::Zero();
      VectorFst<Arc> U1(U);
      RmEpsilon(&U1);
      ShortestDistance(U1, &d, true);
      Weight w1 = U1.Start() < d.size() ? d[U1.Start()] : Weight::Zero();
      CHECK(ApproxEqual(w, w1, kTestDelta));
      RmEpsilonFst<Arc> U2(U);
      ShortestDistance(U2, &d, true);
      Weight w2 = U2.Start() < d.size() ? d[U2.Start()] : Weight::Zero();
      CHECK(ApproxEqual(w, w2, kTestDelta));
    }

    // Checks determinized FSA is equivalent to its input.
    if ((wprops & kSemiring) == kSemiring && tprops & kAcyclic) {
      DeterminizeFst<Arc> D(A);
      CHECK(Equiv(A, D));

      // Checks size(min(det(A))) <= size(det(A))
      // and  min(det(A)) equiv det(A)
      VectorFst<Arc> M(D);
      int n = M.NumStates();
      Minimize(&M);
      CHECK(Equiv(D, M));
      CHECK(M.NumStates() <= n);
    }

    // Checks reweight(T) equiv T
    if (Arc::Type() == LogArc::Type() || Arc::Type() == StdArc::Type()) {
      vector<Weight> potential;
      VectorFst<Arc> RI(T);
      VectorFst<Arc> RF(T);
      while (potential.size() < RI.NumStates())
        potential.push_back((*weight_generator_)());

      Reweight(&RI, potential, REWEIGHT_TO_INITIAL);
      CHECK(Equiv(T, RI));

      Reweight(&RF, potential, REWEIGHT_TO_FINAL);
      CHECK(Equiv(T, RF));
    }

    // Checks pushed FST is equivalent to input FST.
    if ((wprops & kIdempotent) || (tprops & kAcyclic)) {
      // Pushing towards the final state.
      if (wprops & kRightSemiring) {
	VectorFst<Arc> P1;
	Push<Arc, REWEIGHT_TO_FINAL>(T, &P1, kPushLabels);
	CHECK(Equiv(T, P1));

	VectorFst<Arc> P2;
	Push<Arc, REWEIGHT_TO_FINAL>(T, &P2, kPushWeights);
	CHECK(Equiv(T, P2));

	VectorFst<Arc> P3;
	Push<Arc, REWEIGHT_TO_FINAL>(T, &P3, kPushLabels | kPushWeights);
	CHECK(Equiv(T, P3));
      }

      // Pushing towards the initial state.
      if (wprops & kLeftSemiring) {
	VectorFst<Arc> P1;
	Push<Arc, REWEIGHT_TO_INITIAL>(T, &P1, kPushLabels);
	CHECK(Equiv(T, P1));

	VectorFst<Arc> P2;
	Push<Arc, REWEIGHT_TO_INITIAL>(T, &P2, kPushWeights);
	CHECK(Equiv(T, P2));
	VectorFst<Arc> P3;
	Push<Arc, REWEIGHT_TO_INITIAL>(T, &P3, kPushLabels | kPushWeights);
	CHECK(Equiv(T, P3));
      }
    }

    // Checks pruning algorithm
    if ((wprops & (kPath | kCommutative)) == (kPath | kCommutative)) {
      // Checks equiv. of constructive and destructive algorithms
      {
        Weight thresold = (*weight_generator_)();
        VectorFst<Arc> P1(T);
        Prune(&P1, thresold);
        VectorFst<Arc> P2;
        Prune(T, &P2, thresold);
        CHECK(Equiv(P1,P2));
      }
      // Checks prune(reverse) equiv reverse(prune)
      {
        Weight thresold = (*weight_generator_)();
        VectorFst< ReverseArc<Arc> > R;
        VectorFst<Arc> P1(T);
        VectorFst<Arc> P2;
        Prune(&P1, thresold);
        Reverse(T, &R);
        Prune(&R, thresold.Reverse());
        Reverse(R, &P2);
        CHECK(Equiv(P1, P2));
      }
      // Checks:
      //   ShortestDistance(T- prune(T)) > ShortestDistance(T) times Thresold
      {
        Weight thresold = (*weight_generator_)();
        VectorFst<Arc> P;
        Prune(A, &P, thresold);
        DifferenceFst<Arc> C(A, DeterminizeFst<Arc>
                             (RmEpsilonFst<Arc>
                              (MapFst<Arc, Arc,
                               RmWeightMapper<Arc> >
                               (P, RmWeightMapper<Arc>()))));
        Weight sum1 = Times(ShortestDistance(A), thresold);
        Weight sum2 = ShortestDistance(C);
        CHECK(Plus(sum1, sum2) == sum1);
      }
    }
    // Checks synchronize(T) equiv T
    if (tprops & kAcyclic) {
      SynchronizeFst<Arc> S(T);
      CHECK(Equiv(T, S));
    }
  }

  // Tests search operations
  void TestSearch(const Fst<Arc> &T) {
    uint64 wprops = Weight::Properties();

    VectorFst<Arc> A(T);
    Project(&A, PROJECT_INPUT);

    // Checks 1-best weight.
    if ((wprops & (kPath | kRightSemiring)) == (kPath | kRightSemiring)) {
      VectorFst<Arc> path;
      ShortestPath(T, &path);
      Weight tsum = ShortestDistance(T);
      Weight psum = ShortestDistance(path);
      CHECK(ApproxEqual(tsum, psum, kTestDelta));
    }

    // Checks n-best weights
    if ((wprops & (kPath | kSemiring)) == (kPath | kSemiring)) {
      VectorFst<Arc> R(A);
      RmEpsilon(&R);
      int nshortest = rand() % kNumRandomShortestPaths + 2;
      VectorFst<Arc> paths;
      ShortestPath(R, &paths, nshortest, true, false,
                   Weight::Zero(), kNumShortestStates);
      vector<Weight> distance;
      ShortestDistance(paths, &distance, true);
      StateId pstart = paths.Start();
      if (pstart != kNoStateId) {
        ArcIterator< Fst<Arc> > piter(paths, pstart);
        for (; !piter.Done(); piter.Next()) {
          StateId s = piter.Value().nextstate;
          Weight nsum = s < distance.size() ?
              Times(piter.Value().weight, distance[s]) : Weight::Zero();
          VectorFst<Arc> path;
          ShortestPath(R, &path);
          Weight dsum = ShortestDistance(path);
          CHECK(ApproxEqual(nsum, dsum, kTestDelta));
          Map(&path, RmWeightMapper<Arc>());
          VectorFst<Arc> S;
          Difference(R, path, &S);
          R = S;
        }
      }
    }
  }

  // Tests if two FSTS are equivalent by checking if random
  // strings from one FST are transduced the same by both FSTs.
  bool Equiv(const Fst<Arc> &fst1, const Fst<Arc> &fst2) {
    // Checks FSTs for sanity (including property bits).
    CHECK(Verify(fst1));
    CHECK(Verify(fst2));

    UniformArcSelector<Arc> uniform_selector(seed_);
    RandGenOptions< UniformArcSelector<Arc> >
        opts(uniform_selector, kRandomPathLength);
    return RandEquivalent(fst1, fst2, kNumRandomPaths, kTestDelta, opts);
  }

  // Random seed
  int seed_;

  // FST with no states
  VectorFst<Arc> zero_fst_;

  // FST with one state that accepts epsilon.
  VectorFst<Arc> one_fst_;

  // FST with one state that accepts all strings.
  VectorFst<Arc> univ_fst_;

  // Generates weights used in testing.
  WeightGenerator *weight_generator_;

  // Maximum random path length.
  static const int kRandomPathLength = 25;

  // Number of random paths to explore.
  static const int kNumRandomPaths = 100;

  // Maximum number of nshortest paths.
  static const int kNumRandomShortestPaths = 10;

  // Maximum number of nshortest states.
  static const int kNumShortestStates = 10000;

  // Delta for equivalence tests.
  static const float kTestDelta;

  DISALLOW_COPY_AND_ASSIGN(WeightedTester);
};

template <class A, class WG>
const float WeightedTester<A, WG>::kTestDelta = .05;


// This class tests a variety of identities and properties that must
// hold for various algorithms on unweighted FSAs and that are not tested
// by WeightedTester. Only the specialization does anything interesting.
template <class Arc>
class UnweightedTester {
 public:
  UnweightedTester(const Fst<Arc> &zero_fsa, const Fst<Arc> &one_fsa,
                   const Fst<Arc> &univ_fsa) {}

  void Test(const Fst<Arc> &A1, const Fst<Arc> &A2, const Fst<Arc> &A3) {}
};


// Specialization for StdArc. This should work for any commutative,
// idempotent semiring when restricted to the unweighted case
// (being isomorphic to the boolean semiring).
template <>
class UnweightedTester<StdArc> {
 public:
  typedef StdArc Arc;
  typedef Arc::Label Label;
  typedef Arc::StateId StateId;
  typedef Arc::Weight Weight;

  UnweightedTester(const Fst<Arc> &zero_fsa, const Fst<Arc> &one_fsa,
                   const Fst<Arc> &univ_fsa)
      : zero_fsa_(zero_fsa), one_fsa_(one_fsa), univ_fsa_(univ_fsa) {}

  void Test(const Fst<Arc> &A1, const Fst<Arc> &A2, const Fst<Arc> &A3) {
    TestRational(A1, A2, A3);
    TestIntersect(A1, A2, A3);
    TestOptimize(A1);
  }

 private:
  // Tests rational operations with identities
  void TestRational(const Fst<Arc> &A1, const Fst<Arc> &A2,
                    const Fst<Arc> &A3) {

    // Checks the union contains its arguments (destructive).
    {
      VectorFst<Arc> U(A1);
      Union(&U, A2);

      CHECK(Subset(A1, U));
      CHECK(Subset(A2, U));
    }

    // Checks the union contains its arguments (delayed).
    {
      UnionFst<Arc> U(A1, A2);

      CHECK(Subset(A1, U));
      CHECK(Subset(A2, U));
    }

    // Checks if A^n c A* (destructive).
    {
      VectorFst<Arc> C(one_fsa_);
      int n = rand() % 5;
      for (int i = 0; i < n; ++i)
        Concat(&C, A1);

      VectorFst<Arc> S(A1);
      Closure(&S, CLOSURE_STAR);
      CHECK(Subset(C, S));
    }

    // Checks if A^n c A* (delayed).
    {
      int n = rand() % 5;
      Fst<Arc> *C = new VectorFst<Arc>(one_fsa_);
      for (int i = 0; i < n; ++i) {
        ConcatFst<Arc> *F = new ConcatFst<Arc>(*C, A1);
        delete C;
        C = F;
      }
      ClosureFst<Arc> S(A1, CLOSURE_STAR);
      CHECK(Subset(*C, S));
      delete C;
    }
  }

  // Tests intersect-based operations.
  void TestIntersect(const Fst<Arc> &A1, const Fst<Arc> &A2,
                   const Fst<Arc> &A3) {
    VectorFst<Arc> S1(A1);
    VectorFst<Arc> S2(A2);
    VectorFst<Arc> S3(A3);

    ILabelCompare<Arc> comp;

    ArcSort(&S1, comp);
    ArcSort(&S2, comp);
    ArcSort(&S3, comp);

    // Checks the intersection is contained in its arguments.
    {
      IntersectFst<Arc> I1(S1, S2);
      CHECK(Subset(I1, S1));
      CHECK(Subset(I1, S2));
    }

    // Checks union distributes over intersection.
    {
      IntersectFst<Arc> I1(S1, S2);
      UnionFst<Arc> U1(I1, S3);

      UnionFst<Arc> U2(S1, S3);
      UnionFst<Arc> U3(S2, S3);
      ArcSortFst< Arc, ILabelCompare<Arc> > S4(U3, comp);
      IntersectFst<Arc> I2(U2, S4);

      CHECK(Equiv(U1, I2));
    }

    VectorFst<Arc> C1;
    VectorFst<Arc> C2;
    Complement(S1, &C1);
    Complement(S2, &C2);
    ArcSort(&C1, comp);
    ArcSort(&C2, comp);


    // Checks S U S' = Sigma*
    {
      UnionFst<Arc> U(S1, C1);
      CHECK(Equiv(U, univ_fsa_));
    }

    // Checks S n S' = {}
    {
      IntersectFst<Arc> I(S1, C1);
      CHECK(Equiv(I, zero_fsa_));
    }

    // Checks (S1' U S2') == (S1 n S2)'
    {
      UnionFst<Arc> U(C1, C2);

      IntersectFst<Arc> I(S1, S2);
      VectorFst<Arc> C3;
      Complement(I, &C3);
      CHECK(Equiv(U, C3));
    }

    // Checks (S1' n S2') == (S1 U S2)'
    {
      IntersectFst<Arc> I(C1, C2);

      UnionFst<Arc> U(S1, S2);
      VectorFst<Arc> C3;
      Complement(U, &C3);
      CHECK(Equiv(I, C3));
    }
  }

  // Tests optimization operations
  void TestOptimize(const Fst<Arc> &A) {
    // Checks determinized FSA is equivalent to its input.
    {
      DeterminizeFst<Arc> D(A);
      CHECK(Equiv(A, D));
    }

    // Checks minimized FSA is equivalent to its input.
    {
      RmEpsilonFst<Arc> R(A);
      DeterminizeFst<Arc> D(R);
      VectorFst<Arc> M(D);
      Minimize(&M);
      CHECK(Equiv(A, M));
    }
  }

  // Tests if two FSAS are equivalent.
  bool Equiv(const Fst<Arc> &fsa1, const Fst<Arc> &fsa2) {
    // Checks FSAs for sanity (including property bits).
    CHECK(Verify(fsa1));
    CHECK(Verify(fsa2));

    VectorFst<Arc> vfsa1(fsa1);
    VectorFst<Arc> vfsa2(fsa2);
    RmEpsilon(&vfsa1);
    RmEpsilon(&vfsa2);
    DeterminizeFst<Arc> dfa1(vfsa1);
    DeterminizeFst<Arc> dfa2(vfsa2);

    // Test equivalence using union-find algorithm
    bool equiv1 = Equivalent(dfa1, dfa2);

    // Test equivalence by checking if (S1 - S2) U (S2 - S1) is empty
    ILabelCompare<Arc> comp;
    VectorFst<Arc> sdfa1(dfa1);
    ArcSort(&sdfa1, comp);
    VectorFst<Arc> sdfa2(dfa2);
    ArcSort(&sdfa2, comp);

    DifferenceFst<Arc> dfsa1(sdfa1, sdfa2);
    DifferenceFst<Arc> dfsa2(sdfa2, sdfa1);

    VectorFst<Arc> ufsa(dfsa1);
    Union(&ufsa, dfsa2);
    Connect(&ufsa);
    bool equiv2 = ufsa.NumStates() == 0;

    // Check two equivalence tests match
    CHECK((equiv1 && equiv2) || (!equiv1 && !equiv2));

    return equiv1;
  }

  // Tests if FSA1 is a subset of FSA2 (disregarding weights).
  bool Subset(const Fst<Arc> &fsa1, const Fst<Arc> &fsa2) {
    // Checks FSAs (incl. property bits) for sanity
    CHECK(Verify(fsa1));
    CHECK(Verify(fsa2));

    VectorFst<StdArc> vfsa1;
    VectorFst<StdArc> vfsa2;
    RmEpsilon(&vfsa1);
    RmEpsilon(&vfsa2);
    ILabelCompare<StdArc> comp;
    ArcSort(&vfsa1, comp);
    ArcSort(&vfsa2, comp);
    IntersectFst<StdArc> ifsa(vfsa1, vfsa2);
    DeterminizeFst<StdArc> dfa1(vfsa1);
    DeterminizeFst<StdArc> dfa2(ifsa);
    return Equivalent(dfa1, dfa2);
  }

  // Returns complement Fsa
  void Complement(const Fst<Arc> &ifsa, MutableFst<Arc> *ofsa) {
    RmEpsilonFst<Arc> rfsa(ifsa);
    DeterminizeFst<Arc> dfa(rfsa);
    DifferenceFst<Arc> cfsa(univ_fsa_, dfa);
    *ofsa = cfsa;
  }

  // FSA with no states
  VectorFst<Arc> zero_fsa_;

  // FSA with one state that accepts epsilon.
  VectorFst<Arc> one_fsa_;

  // FSA with one state that accepts all strings.
  VectorFst<Arc> univ_fsa_;

  DISALLOW_COPY_AND_ASSIGN(UnweightedTester);
};


// This class tests a variety of identities and properties that must
// hold for various FST algorithms. It randomly generates FSTs, using
// function object 'weight_generator' to select weights. 'WeightTester'
// and 'UnweightedTester' are then called.
template <class Arc, class WeightGenerator>
class AlgoTester {
 public:
  typedef typename Arc::Label Label;
  typedef typename Arc::StateId StateId;
  typedef typename Arc::Weight Weight;

  AlgoTester(WeightGenerator generator, int seed) :
      weight_generator_(generator), seed_(seed) {
      one_fst_.AddState();
      one_fst_.SetStart(0);
      one_fst_.SetFinal(0, Weight::One());

      univ_fst_.AddState();
      univ_fst_.SetStart(0);
      univ_fst_.SetFinal(0, Weight::One());
      for (int i = 0; i < kNumRandomLabels; ++i)
        univ_fst_.AddArc(0, Arc(i, i, Weight::One(), 0));
  }

  void Test() {
    VLOG(1) << "weight type = " << Weight::Type();

    for (int i = 0; i < FLAGS_repeat; ++i) {
      // Random transducers
      VectorFst<Arc> T1;
      VectorFst<Arc> T2;
      VectorFst<Arc> T3;
      RandFst(&T1);
      RandFst(&T2);
      RandFst(&T3);
      WeightedTester<Arc, WeightGenerator>
        weighted_tester(seed_, zero_fst_, one_fst_,
                        univ_fst_, &weight_generator_);
      weighted_tester.Test(T1, T2, T3);

      VectorFst<Arc> A1(T1);
      VectorFst<Arc> A2(T2);
      VectorFst<Arc> A3(T3);
      Project(&A1, PROJECT_OUTPUT);
      Project(&A2, PROJECT_INPUT);
      Project(&A3, PROJECT_INPUT);
      Map(&A1, rm_weight_mapper);
      Map(&A2, rm_weight_mapper);
      Map(&A3, rm_weight_mapper);
      UnweightedTester<Arc> unweighted_tester(zero_fst_, one_fst_, univ_fst_);
      unweighted_tester.Test(A1, A2, A3);
    }
  }

 private:
  // Generates a random FST.
  void RandFst(MutableFst<Arc> *fst) {
    // Determines direction of the arcs wrt state numbering. This way we
    // can force acyclicity when desired.
    enum ArcDirection { ANY_DIRECTION = 0, FORWARD_DIRECTION = 1,
                        REVERSE_DIRECTION = 2, NUM_DIRECTIONS = 3 };

    ArcDirection arc_direction = ANY_DIRECTION;
    if (rand()/(RAND_MAX  + 1.0) < kAcyclicProb)
      arc_direction =  rand() % 2 ? FORWARD_DIRECTION : REVERSE_DIRECTION;

    fst->DeleteStates();
    StateId ns = rand() % kNumRandomStates;

    if (ns == 0)
      return;
    for (StateId s = 0; s < ns; ++s)
      fst->AddState();

    StateId start = rand() % ns;
    fst->SetStart(start);

    size_t na = rand() % kNumRandomArcs;
    for (size_t n = 0; n < na; ++n) {
      StateId s = rand() % ns;
      Arc arc;
      arc.ilabel = rand() % kNumRandomLabels;
      arc.olabel = rand() % kNumRandomLabels;
      arc.weight = weight_generator_();
      arc.nextstate = rand() % ns;

      if (arc_direction == ANY_DIRECTION ||
          (arc_direction == FORWARD_DIRECTION && arc.ilabel > arc.olabel) ||
          (arc_direction == REVERSE_DIRECTION && arc.ilabel < arc.olabel))
        fst->AddArc(s, arc);
    }

    StateId nf = rand() % (ns + 1);
    for (StateId n = 0; n < nf; ++n) {
      StateId s = rand() % ns;
      Weight final = weight_generator_();
      fst->SetFinal(s, final);
    }
    // Checks FST for sanity (including property bits).
    CHECK(Verify(*fst));

    // Get/compute all properties.
    uint64 props = fst->Properties(kFstProperties, true);

    // Select random set of properties to be unknown.
    uint64 mask = 0;
    for (int n = 0; n < 8; ++n) {
      mask |= rand() & 0xff;
      mask <<= 8;
    }
    mask &= ~kTrinaryProperties;
    fst->SetProperties(props & ~mask, mask);
  }

  // Generates weights used in testing.
  WeightGenerator weight_generator_;

  // Random seed
  int seed_;

  // FST with no states
  VectorFst<Arc> zero_fst_;

  // FST with one state that accepts epsilon.
  VectorFst<Arc> one_fst_;

  // FST with one state that accepts all strings.
  VectorFst<Arc> univ_fst_;

  // Mapper to remove weights from an Fst
  RmWeightMapper<Arc> rm_weight_mapper;

  // Maximum number of states in random test Fst.
  static const int kNumRandomStates = 10;

  // Maximum number of arcs in random test Fst.
  static const int kNumRandomArcs = 25;

  // Number of alternative random labels.
  static const int kNumRandomLabels = 5;

  // Probability to force an acyclic Fst
  static const float kAcyclicProb;

  // Maximum random path length.
  static const int kRandomPathLength = 25;

  // Number of random paths to explore.
  static const int kNumRandomPaths = 100;

  DISALLOW_COPY_AND_ASSIGN(AlgoTester);
};

template <class A, class G> const float AlgoTester<A, G>::kAcyclicProb = .25;

}  // namespace fst


using fst::StdArc;
using fst::TropicalWeightGenerator;

using fst::LogArc;
using fst::LogWeightGenerator;

using fst::MinMaxArc;
using fst::MinMaxWeightGenerator;

using fst::StringArc;
using fst::StringWeightGenerator;
using fst::STRING_LEFT;
using fst::STRING_RIGHT;

using fst::GallicArc;
using fst::GallicWeightGenerator;

using fst::LexicographicArc;
using fst::TropicalWeight;
using fst::LexicographicWeightGenerator;

using fst::AlgoTester;

int main(int argc, char **argv) {
  FLAGS_fst_verify_properties = true;
  std::set_new_handler(FailedNewHandler);
  SetFlags(argv[0], &argc, &argv, true);

  int seed = FLAGS_seed >= 0 ? FLAGS_seed : time(0);
  srand(seed);
  LOG(INFO) << "Seed = " << seed;

#ifdef TEST_TROPICAL
  TropicalWeightGenerator tropical_generator(seed, false);
  AlgoTester<StdArc, TropicalWeightGenerator>
    tropical_tester(tropical_generator, seed);
  tropical_tester.Test();
#endif  // TEST_TROPICAL

#ifdef TEST_LOG
  LogWeightGenerator log_generator(seed, false);
  AlgoTester<LogArc, LogWeightGenerator>
    log_tester(log_generator, seed);
  log_tester.Test();
#endif  // TEST_LOG

#ifdef TEST_MINMAX
  MinMaxWeightGenerator minmax_generator(seed, false);
  AlgoTester<MinMaxArc, MinMaxWeightGenerator>
      minmax_tester(minmax_generator, seed);
  minmax_tester.Test();
#endif

#ifdef TEST_LEFT_STRING
  StringWeightGenerator<int> left_string_generator(seed, false);
  AlgoTester<StringArc<>, StringWeightGenerator<int> >
    left_string_tester(left_string_generator, seed);
  left_string_tester.Test();
#endif  // TEST_LEFT_STRING

#ifdef TEST_RIGHT_STRING
  StringWeightGenerator<int, STRING_RIGHT> right_string_generator(seed, false);
  AlgoTester<StringArc<STRING_RIGHT>,
    StringWeightGenerator<int, STRING_RIGHT> >
    right_string_tester(right_string_generator, seed);
  right_string_tester.Test();
#endif  // TEST_RIGHT_STRING

#ifdef TEST_GALLIC
  typedef GallicArc<StdArc> StdGallicArc;
  typedef GallicWeightGenerator<int, TropicalWeightGenerator>
    TropicalGallicWeightGenerator;

  TropicalGallicWeightGenerator tropical_gallic_generator(seed, false);
  AlgoTester<StdGallicArc, TropicalGallicWeightGenerator>nlp_
    gallic_tester(tropical_gallic_generator, seed);
  gallic_tester.Test();
#endif  // TEST_GALLIC

#ifdef TEST_LEXICOGRAPHIC
  typedef LexicographicArc<TropicalWeight, TropicalWeight>
      TropicalLexicographicArc;
  typedef LexicographicWeightGenerator<TropicalWeightGenerator,
      TropicalWeightGenerator> TropicalLexicographicWeightGenerator;
  TropicalLexicographicWeightGenerator lexicographic_generator(seed, false);
  AlgoTester<TropicalLexicographicArc, TropicalLexicographicWeightGenerator>
      lexicographic_tester(lexicographic_generator, seed);
  lexicographic_tester.Test();
#endif  // TEST_LEXICOGRAPHIC

  cout << "PASS" << endl;

  return 0;
}
