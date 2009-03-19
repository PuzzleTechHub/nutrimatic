// mains.h

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
//
// This convenience file includes all Fst main inl.h files.
//

#ifndef FST_MAINS_H__
#define FST_MAINS_H__

#include "./arcsort-main.h"
#include "./closure-main.h"
#include "./compile-main.h"
#include "./compose-main.h"
#include "./concat-main.h"
#include "./connect-main.h"
#include "./convert-main.h"
#include "./determinize-main.h"
#include "./difference-main.h"
#include "./draw-main.h"
#include "./epsnormalize-main.h"
#include "./equal-main.h"
#include "./equivalent-main.h"
#include "./encode-main.h"
#include "./info-main.h"
#include "./intersect-main.h"
#include "./invert-main.h"
#include "./map-main.h"
#include "./minimize-main.h"
#include "./print-main.h"
#include "./project-main.h"
#include "./prune-main.h"
#include "./push-main.h"
#include "./randgen-main.h"
#include "./relabel-main.h"
#include "./replace-main.h"
#include "./reverse-main.h"
#include "./reweight-main.h"
#include "./rmepsilon-main.h"
#include "./shortest-distance-main.h"
#include "./shortest-path-main.h"
#include "./synchronize-main.h"
#include "./topsort-main.h"
#include "./union-main.h"

namespace fst {

// Invokes REGISTER_FST_MAIN on all mains.
template <class A> void RegisterFstMains() {
  REGISTER_FST_MAIN(ArcSortMain, A);
  REGISTER_FST_MAIN(ClosureMain, A);
  REGISTER_FST_MAIN(CompileMain, A);
  REGISTER_FST_MAIN(ComposeMain, A);
  REGISTER_FST_MAIN(ConcatMain, A);
  REGISTER_FST_MAIN(ConnectMain, A);
  REGISTER_FST_MAIN(ConvertMain, A);
  REGISTER_FST_MAIN(DeterminizeMain, A);
  REGISTER_FST_MAIN(DifferenceMain, A);
  REGISTER_FST_MAIN(DrawMain, A);
  REGISTER_FST_MAIN(EquivalentMain, A);
  REGISTER_FST_MAIN(EncodeMain, A);
  REGISTER_FST_MAIN(EpsNormalizeMain, A);
  REGISTER_FST_MAIN(EqualMain, A);
  REGISTER_FST_MAIN(InfoMain, A);
  REGISTER_FST_MAIN(IntersectMain, A);
  REGISTER_FST_MAIN(InvertMain, A);
  REGISTER_FST_MAIN(MapMain, A);
  REGISTER_FST_MAIN(MinimizeMain, A);
  REGISTER_FST_MAIN(PrintMain, A);
  REGISTER_FST_MAIN(ProjectMain, A);
  REGISTER_FST_MAIN(PruneMain, A);
  REGISTER_FST_MAIN(PushMain, A);
  REGISTER_FST_MAIN(RandGenMain, A);
  REGISTER_FST_MAIN(RelabelMain, A);
  REGISTER_FST_MAIN(ReplaceMain, A);
  REGISTER_FST_MAIN(ReverseMain, A);
  REGISTER_FST_MAIN(ReweightMain, A);
  REGISTER_FST_MAIN(RmEpsilonMain, A);
  REGISTER_FST_MAIN(ShortestDistanceMain, A);
  REGISTER_FST_MAIN(ShortestPathMain, A);
  REGISTER_FST_MAIN(SynchronizeMain, A);
  REGISTER_FST_MAIN(TopSortMain, A);
  REGISTER_FST_MAIN(UnionMain, A);
}

// Convenience macro to invoking RegisterFstMains.
#define REGISTER_FST_MAINS(A) fst::RegisterFstMains<A>();

}

#endif  // FST_MAINS_H__
