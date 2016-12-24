#include "index.h"
#include "search.h"
#include "expr.h"

#include "fst/arcsort.h"
#include "fst/closure.h"
#include "fst/concat.h"
#include "fst/equivalent.h"
#include "fst/intersect.h"
#include "fst/union.h"

#include <stdio.h>

#include <vector>
#include <utility>

using namespace fst;

struct AnagramPart {
  StdVectorFst expr;
  int count;
  int group;
};

static void CollapseIdentical(vector<AnagramPart>* parts) {
  for (size_t i = 0; i < parts->size(); ++i) {
    size_t jin = i + 1, jout = i + 1;
    while (jin < parts->size()) {
      if (Equivalent((*parts)[i].expr, (*parts)[jin].expr)) {
        ++(*parts)[i].count;
        ++jin;
      } else {
        (*parts)[jout++] = (*parts)[jin++];
      }
    }
    parts->resize(jout);
  }
}

static void MakeExpr(vector<AnagramPart> const& parts, StdMutableFst* out) {
  vector<StdVectorFst> to_intersect;

  StdVectorFst any;
  int total = 0;
  for (size_t i = 0; i < parts.size(); ++i) {
    Union(&any, parts[i].expr);
    total += parts[i].count;
  }


  StdVectorFst has_length;
  has_length.SetStart(has_length.AddState());
  has_length.SetFinal(has_length.Start(), StdArc::Weight::One());
  for (int i = 0; i < total; ++i) Concat(&has_length, any);
  to_intersect.push_back(has_length);

  for (size_t i = 0; i < parts.size(); ++i) {
    StdVectorFst others;
    for (size_t j = 0; j < parts.size(); ++j) {
      if (j != i) Union(&others, parts[j].expr);
    }
    Closure(&others, CLOSURE_STAR);

    StdVectorFst contains_part = others;
    for (int n = 0; n < parts[i].count; ++n) {
      Concat(&contains_part, parts[i].expr);
      Concat(&contains_part, others);
    }
    to_intersect.push_back(contains_part);
  }

  IntersectExprs(to_intersect, out);
}

const char *ParseAnagram(const char *p, StdMutableFst* out, bool quoted) {
  if (p == NULL) return NULL;

  vector<AnagramPart> parts;
  while (*p != '>') {
    StdVectorFst expr;
    p = ParsePiece(p, &expr, quoted);
    if (p == NULL) return NULL;

    AnagramPart part;
    OptimizeExpr(expr, &part.expr);
    part.count = 1;
    part.group = parts.size();
    parts.push_back(part);
  }

  CollapseIdentical(&parts);

  if (getenv("DEBUG_FST") != NULL) {
    fprintf(stderr, "anagram: %zd unique parts\n", parts.size());
    for (size_t i = 0; i < parts.size(); ++i) {
      fprintf(stderr, "  #%zu: %d x %d states\n", i,
          parts[i].count,
          parts[i].expr.NumStates());
    }
  }

  MakeExpr(parts, out);
  return p;
}
