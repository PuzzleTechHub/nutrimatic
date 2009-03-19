#include "index.h"
#include "search.h"
#include "expr.h"

#include "fst/vector-fst.h"

#include <assert.h>

using namespace fst;

ExprFilter::ExprFilter(StdFst const& raw) {
  StdVectorFst optimized;
  OptimizeExpr(raw, &optimized);
  if (getenv("DEBUG_FST") != NULL) {
    optimized.Write(getenv("DEBUG_FST"));
  }

  if (optimized.NumStates() == 0) {
    accepting.resize(1, false);
    for (int c = 0; c <= UCHAR_MAX; ++c) next[c].resize(1, -1);
    start_state = 0;
    return;
  }

  accepting.resize(optimized.NumStates());
  for (int c = 0; c <= UCHAR_MAX; ++c) next[c].resize(optimized.NumStates(), -1);
  start_state = optimized.Start();
  assert(start_state >= 0 && start_state < accepting.size());

  for (StateIterator<StdFst> si(optimized); !si.Done(); si.Next()) {
    State s = si.Value();
    assert(s >= 0 && s < accepting.size());
    accepting[s] = (optimized.Final(s) != StdArc::Weight::Zero());
    for (ArcIterator<StdFst> ai(optimized, s); !ai.Done(); ai.Next()) {
      StdArc const& arc = ai.Value();
      assert(arc.ilabel > 0 && arc.ilabel <= UCHAR_MAX);
      assert(arc.nextstate >= 0 && arc.nextstate < next[arc.ilabel].size());
      next[arc.ilabel][s] = arc.nextstate;
    }
  }
}
