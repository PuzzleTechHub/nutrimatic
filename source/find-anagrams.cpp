#include "index.h"
#include "search.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

class AnagramFilter: public SearchFilter {
 public:
  AnagramFilter(char const* letters) {
    for (size_t i = 0; i < sizeof(count) / sizeof(State); ++i) count[i] = 0;
    while (*letters) ++count[(unsigned char) *letters++];

    product = 1;
    for (size_t i = 0; i < sizeof(count) / sizeof(State); ++i) {
      if (0 == count[i]) {
        value[i] = 0;
      } else if (product * count[i] < product) {
        fputs("anagram too long\n", stderr);
        exit(1);
      } else {
	++count[i];
        value[i] = product;
        product *= count[i];
        count[i] *= value[i];
      }
    }
  }

  bool is_accepting(State state) const {
    return (state == product);
  }

  bool has_transition(State from, char ch, State* to) const {
    if (ch == ' ') {
      *to = (from == product - 1) ? product : from;
      return true;
    }

    State v = value[(unsigned char) ch];
    if (v == 0) return false;

    State next = from + v;
    if (next % count[(unsigned char) ch] < v) return false;

    *to = next;
    return true;
  }

 private:
  State count[256];
  State value[256];
  State product;
};

int main(int argc, char *argv[]) {
  if (argc != 3) {
    fprintf(stderr, "usage: %s input.index letters\n", argv[0]);
    return 2;
  }

  FILE *fp = fopen(argv[1], "rb");
  if (fp == NULL) {
    fprintf(stderr, "error: can't open \"%s\"\n", argv[1]);
    return 1;
  }

  IndexReader reader(fp);
  AnagramFilter filter(argv[2]);
  SearchDriver driver(&reader, &filter, 0, 1e-6);
  PrintAll(&driver);
  return 0;
}
