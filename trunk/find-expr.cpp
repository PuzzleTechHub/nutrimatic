#include "index.h"
#include "search.h"
#include "expr.h"

#include "fst/concat.h"

#include <stdio.h>

using namespace fst;

int main(int argc, char *argv[]) {
  if (argc != 3 || strlen(argv[2]) == 0) {
    fprintf(stderr, "usage: %s input.index expression\n", argv[0]);
    return 2;
  }

  SymbolTable *chars = new SymbolTable("chars");
  chars->AddSymbol("epsilon", 0);
  chars->AddSymbol("space", ' ');
  for (int i = 33; i <= 127; ++i)
    chars->AddSymbol(string(1, i), i);

  StdVectorFst fst;
  fst.SetInputSymbols(chars);
  fst.SetOutputSymbols(chars);

  const char *p = ParseExpr(argv[2], &fst, false);
  if (p == NULL || *p != '\0') {
    fprintf(stderr, "error: can't parse \"%s\"\n", p ? p : argv[2]);
    return 2;
  }

  StdVectorFst space;
  ParseExpr(" ", &space, true);

  StdVectorFst final;
  ParseExpr(" ?", &final, true);
  Concat(&final, fst);
  Concat(&final, space);

  ExprFilter filter(final);
  SearchFilter::State start;
  if (!filter.has_transition(filter.start(), ' ', &start)) {
    fprintf(stderr, "error: \"%s\" can't match anything\n", argv[2]);
    return 1;
  }

  FILE *fp = fopen(argv[1], "rb");
  if (fp == NULL) {
    fprintf(stderr, "error: can't open \"%s\"\n", argv[1]);
    return 1;
  }

  IndexReader reader(fp);
  SearchDriver driver(&reader, &filter, start, 1e-6);
  PrintAll(&driver);
  return 0;
}
