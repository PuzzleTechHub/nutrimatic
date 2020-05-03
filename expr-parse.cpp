#include "index.h"
#include "search.h"
#include "expr.h"

#include "fst/closure.h"
#include "fst/concat.h"
#include "fst/union.h"
#include "fst/vector-fst.h"

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <vector>

using namespace fst;
using std::vector;

typedef StdArc::StateId State;
typedef StdArc::Weight Weight;

const char *ParseExpr(const char *p, StdMutableFst* fst, bool quoted) {
  p = ParseBranch(p, fst, quoted);
  while (p != NULL && *p == '|') {
    StdVectorFst branch;
    p = ParseBranch(p + 1, &branch, quoted);
    Union(fst, branch);
  }
  return p;
}

const char *ParseBranch(const char *p, StdMutableFst* fst, bool quoted) {
  vector<StdVectorFst> to_intersect;
  StdVectorFst first;
  p = ParseFactor(p, &first, quoted);
  to_intersect.push_back(first);
  while (p != NULL && *p == '&') {
    StdVectorFst next;
    p = ParseFactor(p + 1, &next, quoted);
    to_intersect.push_back(next);
  }
  IntersectExprs(to_intersect, fst);
  return p;
}

const char *ParseFactor(const char *p, StdMutableFst* fst, bool quoted) {
  fst->SetStart(fst->AddState());
  fst->SetFinal(fst->Start(), Weight::One());
  for (;;) {
    StdVectorFst piece;
    const char *n = ParsePiece(p, &piece, quoted);
    if (n == NULL) return p;
    Concat(fst, piece);
    p = n;
  }
}

const char *ParsePiece(const char *p, StdMutableFst* fst, bool quoted) {
  StdVectorFst one;
  p = ParseAtom(p, &one, quoted);
  if (p == NULL) return NULL;

  int min, max;
  if (*p == '*') {
    min = 0;
    max = INT_MAX;
    ++p;
  } else if (*p == '+') {
    min = 1;
    max = INT_MAX;
    ++p;
  } else if (*p == '?') {
    min = 0;
    max = 1;
    ++p;
  } else if (*p == '{') {
    min = strtoul(p + 1, (char**) &p, 10);
    if (*p == ',' && *(p + 1) == '}') {
      max = INT_MAX;
      ++p;
    } else if (*p == ',') {
      max = strtoul(p + 1, (char**) &p, 10);
    } else {
      max = min;
    }
    if (*p != '}' || max < min || (max > 255 && max < INT_MAX)) return NULL;
    ++p;
  } else {
    min = max = 1;
  }

  StdVectorFst many;
  many.SetStart(many.AddState());
  many.SetFinal(many.Start(), Weight::One());

  assert(max >= min && min >= 0);
  for (int i = 0; i <= min || (i <= max && max < INT_MAX); ++i) {
    if (i >= min) Union(fst, many);
    Concat(&many, one);
  }

  if (max >= INT_MAX) {
    Closure(&one, CLOSURE_STAR);
    Concat(&many, one);
    Union(fst, many);
  }

  return p;
}

const char *ParseAtom(const char *p, StdMutableFst* fst, bool quoted) {
  if (p == NULL) return NULL;

  if (*p == '"' && !quoted) {
    p = ParseExpr(p + 1, fst, true);
    if (p == NULL || *p != '"') return NULL;
    return p + 1;
  } else if (*p == '(') {
    p = ParseExpr(p + 1, fst, quoted);
    if (p == NULL || *p != ')') return NULL;
    return p + 1;
  } else if (*p == '<') {
    p = ParseAnagram(p + 1, fst, quoted);
    if (p == NULL || *p != '>') return NULL;
    return p + 1;
  }

  vector<char> chars;
  bool negate = false;

  if (*p == '[') {
    if (*++p == '^') {
      negate = true;
      ++p;
    }
    while (*p != ']') {
      if (*p == '-') {
        int first = (unsigned char) *(p - 1);
        int last = (unsigned char) *(p + 1);
        for (int c = first + 1; c <= last; ++c) {
          if ((c < 'a' || c > 'z') && (c < '0' || c > '9') && c != ' ') {
            return NULL;
          } else {
            chars.push_back(c);
          }
        }
        p += 2;
      } else {
        p = ParseCharClass(p, &chars);
        if (p == NULL) return p;
      }
    }
    ++p;
  } else {
    p = ParseCharClass(p, &chars);
    if (p == NULL) return NULL;
  }

  State start = fst->AddState(), final = fst->AddState();
  fst->SetStart(start);
  fst->SetFinal(final, Weight::One());
  if (negate) {
    vector<char> all;
    ParseCharClass(".", &all);
    for (int i = 0; i < all.size(); ++i)
      if (find(chars.begin(), chars.end(), all[i]) == chars.end())
        fst->AddArc(start, StdArc(all[i], all[i], Weight::One(), final));
  } else {
    for (int i = 0; i < chars.size(); ++i)
      fst->AddArc(start, StdArc(chars[i], chars[i], Weight::One(), final));
  }

  if (!quoted) {
    fst->AddArc(start, StdArc(' ', ' ', Weight::One(), start));
    fst->AddArc(final, StdArc(' ', ' ', Weight::One(), final));
  }

  return p;
}

const char *ParseCharClass(const char *p, vector<char>* out) {
  if (p == NULL) return NULL;
  if ((*p >= 'a' && *p <= 'z') || (*p >= '0' && *p <= '9') || *p == ' ') {
    out->push_back(*p);
  } else if (*p == '-') {
    out->push_back(0);
    out->push_back(' ');
  } else if (*p == '.') {
    for (int ch = '0'; ch <= '9'; ++ch) out->push_back(ch);
    for (int ch = 'a'; ch <= 'z'; ++ch) out->push_back(ch);
    out->push_back(' ');
  } else if (*p == '_') {
    for (int ch = '0'; ch <= '9'; ++ch) out->push_back(ch);
    for (int ch = 'a'; ch <= 'z'; ++ch) out->push_back(ch);
  } else if (*p == '#') {
    for (int ch = '0'; ch <= '9'; ++ch) out->push_back(ch);
  } else if (*p == 'A') {
    for (int ch = 'a'; ch <= 'z'; ++ch) out->push_back(ch);
  } else if (*p == 'C') {
    for (int ch = 'a'; ch <= 'z'; ++ch)
      if (!strchr("aeiou", ch)) out->push_back(ch);
  } else if (*p == 'V') {
    for (int ch = 'a'; ch <= 'z'; ++ch)
      if (strchr("aeiou", ch)) out->push_back(ch);
  } else {
    return NULL;
  }
  return p + 1;
}
