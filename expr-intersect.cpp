#include "index.h"
#include "search.h"
#include "expr.h"

#include "fst/arcsort.h"
#include "fst/intersect.h"
#include "fst/vector-fst.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

using namespace fst;
using std::vector;

void IntersectExprs(
    vector<StdVectorFst> const& in,
    StdMutableFst* out) {
  if (in.size() == 1) {
    *out = in[0];
    return;
  }

  vector<StdVectorFst> input = in, output;
  while (input.size() > 1) {
    assert(output.empty());
    if (input.size() % 2 > 0) {
      output.push_back(input[input.size() - 1]);
    }
    for (size_t i = 0; i + 1 < input.size(); i += 2) {
      StdVectorFst a, b;
      OptimizeExpr(input[i], &a);
      OptimizeExpr(input[i + 1], &b);
      ArcSort(&a, StdILabelCompare());

      clock_t t1 = clock();
      StdVectorFst merged;
      Intersect(a, b, &merged);
      clock_t t2 = clock();

      if (getenv("DEBUG_FST") != NULL) {
        fprintf(stderr, "intersect(%.2fs): %d & %d => %d\n",
            double(t2 - t1) / CLOCKS_PER_SEC,
            a.NumStates(), b.NumStates(), merged.NumStates());
      }

      output.push_back(merged);
    }

    input.clear();
    input.swap(output);
  }

  if (input.size() == 1) *out = input[0];
}
