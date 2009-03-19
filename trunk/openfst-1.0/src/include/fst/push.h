// push.h

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
// Class to reweight/push an FST.

#ifndef FST_LIB_PUSH_H__
#define FST_LIB_PUSH_H__

#include <vector>
#include <fst/factor-weight.h>
#include <fst/fst.h>
#include <fst/map.h>
#include <fst/reweight.h>
#include <fst/shortest-distance.h>

namespace fst {

// Pushes the weights in FST in the direction defined by TYPE.  If
// pushing towards the initial state, the sum of the weight of the
// outgoing transitions and final weight at a non-initial state is
// equal to One() in the resulting machine.  If pushing towards the
// final state, the same property holds on the reverse machine.
//
// Weight needs to be left distributive when pushing towards the
// initial state and right distributive when pushing towards the final
// states.
template <class Arc>
void Push(MutableFst<Arc> *fst, ReweightType type, float delta = kDelta) {
  vector<typename Arc::Weight> distance;
  ShortestDistance(*fst, &distance, type == REWEIGHT_TO_INITIAL, delta);
  Reweight(fst, distance, type);
}


const uint32 kPushWeights = 0x0001;
const uint32 kPushLabels =  0x0002;

// OFST obtained from IFST by pushing weights and/or labels according
// to PTYPE in the direction defined by RTYPE.  Weight needs to be
// left distributive when pushing weights towards the initial state
// and right distributive when pushing weights towards the final
// states.
template <class Arc, ReweightType rtype>
void Push(const Fst<Arc> &ifst,
          MutableFst<Arc> *ofst,
          uint32 ptype,
          float delta = kDelta) {

  if (ptype == kPushWeights) {
    *ofst = ifst;
    Push(ofst, rtype, delta);
  } else if (ptype & kPushLabels) {
    const StringType stype = rtype == REWEIGHT_TO_INITIAL
                             ? STRING_LEFT
                             : STRING_RIGHT;
    vector<typename GallicArc<Arc, stype>::Weight> gdistance;
    VectorFst< GallicArc<Arc, stype> > gfst;
    Map(ifst, &gfst, ToGallicMapper<Arc, stype>());
    if (ptype == (kPushWeights | kPushLabels)) {
      ShortestDistance(gfst, &gdistance, rtype == REWEIGHT_TO_INITIAL, delta);
    } else {
      MapFst<Arc, Arc, RmWeightMapper<Arc> >
        uwfst(ifst, RmWeightMapper<Arc>());
      MapFst<Arc, GallicArc<Arc, stype>, ToGallicMapper<Arc, stype> >
        guwfst(uwfst, ToGallicMapper<Arc, stype>());
      ShortestDistance(guwfst, &gdistance, rtype == REWEIGHT_TO_INITIAL, delta);
    }
    Reweight(&gfst, gdistance, rtype);
    FactorWeightFst< GallicArc<Arc, stype>, GallicFactor<typename Arc::Label,
      typename Arc::Weight, stype> > fwfst(gfst);
    Map(fwfst, ofst, FromGallicMapper<Arc, stype>());
    ofst->SetOutputSymbols(ifst.OutputSymbols());
  } else {
    LOG(WARNING) << "Push: pushing type is set to 0: "
                 << "pushing neither labels nor weights.";
    *ofst = ifst;
  }
}

}  // namespace fst

#endif /* FST_LIB_PUSH_H_ */
