#include "index.h"
#include "search.h"
#include "expr.h"

#include "fst/concat.h"

#include <algorithm>
#include <string>
#include <vector>

#include <stdio.h>
#include <stdlib.h>

using namespace fst;

static void TestIndex(const char *expr, const char *yes, const char *no) {
  // Write index

  FILE *fp = fopen("test-expr.index", "wb");
  if (fp == NULL) {
    fprintf(stderr, "FAIL: can't write test-expr.index\n");
    exit(1);
  }

  IndexWriter writer(fp);
  std::vector<std::string> str;
  if (yes != NULL) str.push_back(yes);
  if (no != NULL) str.push_back(no);
  std::sort(str.begin(), str.end());
  for (size_t i = 0; i < str.size(); ++i) writer.next(str[i].c_str(), 0, 1);
  writer.next(NULL, 0, 0);
  fclose(fp);

  // Parse expression

  if (getenv("DEBUG_FST") != NULL) fprintf(stderr, "### [%s]\n", expr);

  StdVectorFst fst;
  const char *p = ParseExpr(expr, &fst, false);
  if (p == NULL || *p != '\0') {
    fprintf(stderr, "FAIL: can't parse \"%s\"\n", p ? p : expr);
    exit(1);
  }

  // Read index

  fp = fopen("test-expr.index", "rb");
  if (fp == NULL) {
    fprintf(stderr, "FAIL: can't open test-expr.index\n");
    exit(1);
  }

  IndexReader reader(fp);
  ExprFilter filter(fst);
  SearchDriver sd(&reader, &filter, filter.start(), 1e-6);
  sd.next();

  // Verify results

  if (sd.text == NULL && yes == NULL) {
    if (getenv("DEBUG_FST") != NULL) fprintf(stderr, "-> NULL (ok)\n");
    return;
  }

  if (sd.text == NULL) {
    fprintf(stderr, "FAIL: [%s] -> NULL (expected \"%s\")\n", expr, yes);
    exit(1);
  }

  if (yes == NULL) {
    fprintf(stderr, "FAIL: [%s] -> \"%s\" (expected NULL)\n", expr, sd.text);
    exit(1);
  }

  if (strcmp(yes, sd.text)) {
    fprintf(stderr, "FAIL: [%s] -> \"%s\" (expected \"%s\")\n", expr, sd.text, yes);
    exit(1);
  }

  if (getenv("DEBUG_FST") != NULL) fprintf(stderr, "-> \"%s\" (ok)\n", yes);

  if (sd.text != NULL) {
    double score = sd.score;
    sd.next();
    if (sd.text != NULL && sd.score >= score) {
      fprintf(stderr, "FAIL: [%s] -> \"%s\" (extra)\n", expr, sd.text);
      exit(1);
    }
  }

  fclose(fp);
  remove("test-expr.index");
}

int main(int argc, char *argv[]) {
  TestIndex(
      "foo&bar",
      NULL,
      " ");

  TestIndex(
      "\"(((((m?o)?c)?h)?i)t?)_(h(a(t(o(ry?)?)?)?)?)?&_{5,}\" ",
      "chitchat ",
      "itch ");

  TestIndex(
      "(\"<(-may)?(-sit)?(tit)?(ble)?(com)?(iks)?(ial)?(im-b)?(-mon)?>\"&_{18}) ",
      "mayim bialiks sitcom ",
      "mayim bialiks common ");

  TestIndex(
      "([aehimnprsw]*&_*a_*&_*e_*&_*h_*&_*i_*&_*m_*&_*n_*&_*p_*&_*r_*&_*s_*&_*w_*) ",
      "new hampshire ",
      "minesweeper ship ");

  TestIndex(
      "<eelqsuuu> ",
      "equuleus ",
      "equus ");

  TestIndex(
      "(c?h?a?r?m?&____)(e?l?t?o?n?&____)(c?h?e?s?t?&____)(o?n?e?&__) ",
      "charlton heston ",
      "charmton heston ");

  TestIndex(
      "(<(cerb)?(ecto)?(lonm)?(ddog)?(fblo)?(iero)?(skey)?(ells)?(dwhi)?(atra)?(subj)?(odan)?(thel)?>&_{24}) ",
      "subject of blood and whiskey ",
      "subject of blood and whisubj ");

  TestIndex(
      "\"<(cs)(dy)(er)(i)(mo)(n)(th)(__?)>\" ",
      "thermodynamics ",
      "thermodyanmics ");

  TestIndex(
      "(<waterhegm>&_*w_*a_*t_*e_*r_*) ",
      "wheat germ ",
      "merge what ");

  TestIndex(
      "<het><ral><seg><tan><rut><bla><oody><afl><ndi><cin><awe><ter> ",
      "the largest natural body of land in ice water ",
      "the largest natural body of water in iceland ");

  return 0;
}
