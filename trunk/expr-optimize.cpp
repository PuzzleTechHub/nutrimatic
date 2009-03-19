#include "index.h"
#include "search.h"
#include "expr.h"

#include "fst/determinize.h"
#include "fst/minimize.h"
#include "fst/rmepsilon.h"
#include "fst/vector-fst.h"
#include "fst/verify.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

using namespace fst;

void OptimizeExpr(StdFst const& input, StdMutableFst* output) {
  StdVectorFst tmp(input);
  clock_t t1 = clock();
  int n1 = tmp.NumStates();

  RmEpsilon(&tmp);
  clock_t t2 = clock();
  int n2 = tmp.NumStates();

  Determinize(tmp, output);
  clock_t t3 = clock();
  int n3 = output->NumStates();

  Minimize(output);
  clock_t t4 = clock();
  int n4 = output->NumStates();

  if (getenv("DEBUG_FST") != NULL) {
    fprintf(stderr,
        "optimize(%.2fs): %d rmeps(%.2fs) %d det(%.2fs) %d min(%.2fs) %d\n",
        double(t4 - t1) / CLOCKS_PER_SEC, n1,
        double(t2 - t1) / CLOCKS_PER_SEC, n2,
        double(t3 - t2) / CLOCKS_PER_SEC, n3,
        double(t4 - t3) / CLOCKS_PER_SEC, n4);
  }
}
