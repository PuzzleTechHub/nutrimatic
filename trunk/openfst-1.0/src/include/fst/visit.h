// visit.h

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
// Queue-dependent visitation of finite-state transducers. See also
// dfs-visit.h.

#ifndef FST_LIB_VISIT_H__
#define FST_LIB_VISIT_H__

#include <fst/arcfilter.h>
#include <fst/mutable-fst.h>

namespace fst {

// Visitor Interface - class determines actions taken during a visit.
// If any of the boolean member functions return false, the visit is
// aborted by first calling FinishState() on all unfinished (grey)
// states and then calling FinishVisit().
//
// Note this is more general than the visitor interface in
// dfs-visit.h but lacks some DFS-specific behavior.
//
// template <class Arc>
// class Visitor {
//  public:
//   typedef typename Arc::StateId StateId;
//
//   Visitor(T *return_data);
//   // Invoked before visit
//   void InitVisit(const Fst<Arc> &fst);
//   // Invoked when state discovered (2nd arg is visitation root)
//   bool InitState(StateId s, StateId root);
//   // Invoked when arc to white/undiscovered state examined
//   bool WhiteArc(StateId s, const Arc &a);
//   // Invoked when arc to grey/unfinished state examined
//   bool GreyArc(StateId s, const Arc &a);
//   // Invoked when arc to black/finished state examined
//   bool BlackArc(StateId s, const Arc &a);
//   // Invoked when state finished.
//   void FinishState(StateId s);
//   // Invoked after visit
//   void FinishVisit();
// };

// Performs queue-dependent visitation. Visitor class argument
// determines actions and contains any return data. ArcFilter
// determines arcs that are considered.
//
// Note this is more general than DfsVisit() in dfs-visit.h but lacks
// some DFS-specific Visitor behavior.
template <class Arc, class V, class Q, class ArcFilter>
void Visit(const Fst<Arc> &fst, V *visitor, Q *queue, ArcFilter filter) {

  typedef typename Arc::StateId StateId;
  typedef ArcIterator< Fst<Arc> > AIterator;

  visitor->InitVisit(fst);

  StateId start = fst.Start();
  if (start == kNoStateId) {
    visitor->FinishVisit();
    return;
  }

  // An Fst state's visit color
  const unsigned kWhiteState =  0x01;  // Undiscovered
  const unsigned kGreyState =   0x02;  // Discovered & unfinished
  const unsigned kBlackState =  0x04;  // Finished

  // We destroy an iterator as soon as possible and mark it so
  const unsigned kArcIterDone = 0x08;  // Arc iterator done and destroyed

  StateId nstates = CountStates(fst);

  unsigned char *state_status = new unsigned char[nstates];
  AIterator **arc_iterator = new AIterator *[nstates];
  for (StateId s = 0; s < nstates; ++s) {
    state_status[s] = kWhiteState;
    arc_iterator[s] = 0;
  }

  // Continues visit while true
  bool visit = true;

  // Iterates over trees in visit forest.
  for (StateId root = start; visit && root < nstates;) {
    visit = visitor->InitState(root, root);
    state_status[root] = kGreyState;
    queue->Enqueue(root);
    while (!queue->Empty()) {
      StateId s = queue->Head();
      // Creates arc iterator if needed.
      if (arc_iterator[s] == 0 && !(state_status[s] & kArcIterDone) && visit)
        arc_iterator[s] = new AIterator(fst, s);
      // Deletes arc iterator if done.
      AIterator *aiter = arc_iterator[s];
      if (aiter && (aiter->Done() || !visit)) {
        delete aiter;
        arc_iterator[s] = 0;
        state_status[s] |= kArcIterDone;
      }
      // Dequeues state and marks black if done
      if (state_status[s] & kArcIterDone) {
        queue->Dequeue();
        visitor->FinishState(s);
        state_status[s] = kBlackState;
        continue;
      }

      const Arc &arc = aiter->Value();
      // Visits respective arc types
      if (filter(arc)) {
        // Enqueues destination state and marks grey if white
        if (state_status[arc.nextstate] == kWhiteState) {
          visit = visitor->WhiteArc(s, arc);
          if (!visit) continue;
          visit = visitor->InitState(arc.nextstate, root);
          state_status[arc.nextstate] = kGreyState;
          queue->Enqueue(arc.nextstate);
        } else if (state_status[arc.nextstate] == kBlackState) {
          visit = visitor->BlackArc(s, arc);
        } else {
          visit = visitor->GreyArc(s, arc);
        }
      }
      aiter->Next();
      // Destroys an iterator ASAP for efficiency.
      if (aiter->Done()) {
        delete aiter;
        arc_iterator[s] = 0;
        state_status[s] |= kArcIterDone;
      }
    }
    // Finds next tree root
    for (root = root == start ? 0 : root + 1;
         root < nstates && state_status[root] != kWhiteState;
         ++root);
  }
  visitor->FinishVisit();
  delete[] state_status;
  delete[] arc_iterator;
}


template <class Arc, class V, class Q>
inline void Visit(const Fst<Arc> &fst, V *visitor, Q* queue) {
  Visit(fst, visitor, queue, AnyArcFilter<Arc>());
}


template <class A>
class CopyVisitor {
 public:
  typedef A Arc;
  typedef typename A::StateId StateId;

  CopyVisitor(MutableFst<Arc> *ofst) : ifst_(0), ofst_(ofst) {}

  void InitVisit(const Fst<A> &ifst) {
    ifst_ = &ifst;
    ofst_->DeleteStates();
    ofst_->SetStart(ifst_->Start());
  }

  bool InitState(StateId s, StateId) {
    while (ofst_->NumStates() <= s)
      ofst_->AddState();
    return true;
  }

  bool WhiteArc(StateId s, const Arc &arc) {
    ofst_->AddArc(s, arc);
    return true;
  }

  bool GreyArc(StateId s, const Arc &arc) {
    ofst_->AddArc(s, arc);
    return true;
  }

  bool BlackArc(StateId s, const Arc &arc) {
    ofst_->AddArc(s, arc);
    return true;
  }

  void FinishState(StateId s) {
    ofst_->SetFinal(s, ifst_->Final(s));
  }

  void FinishVisit() {}

 private:
  const Fst<Arc> *ifst_;
  MutableFst<Arc> *ofst_;
};

}  // namespace fst

#endif  // FST_LIB_VISIT_H__
