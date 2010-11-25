#include "index.h"
#include "search.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

class PhoneFilter: public SearchFilter {
 public:
  PhoneFilter(char const* digits): num(digits), len(strlen(digits)) { }

  bool is_accepting(State state) const {
    assert(state >= 0 && state <= len + 1);
    return state == len + 1;
  }

  bool has_transition(State from, char ch, State* to) const {
    assert(from >= 0 && from <= len + 1);
    if (from == len + 1) return false;

    switch (ch) {
      case ' ':
        *to = (from == len) ? from + 1 : from;
        return true;

      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
        if (num[from] != ch) return false;
        break;

      case 'a': case 'b': case 'c':
        if (num[from] != '2') return false;
        break;

      case 'd': case 'e': case 'f':
        if (num[from] != '3') return false;
        break;

      case 'g': case 'h': case 'i':
        if (num[from] != '4') return false;
        break;

      case 'j': case 'k': case 'l':
        if (num[from] != '5') return false;
        break;

      case 'm': case 'n': case 'o':
        if (num[from] != '6') return false;
        break;

      case 'p': case 'q': case 'r': case 's':
        if (num[from] != '7') return false;
        break;

      case 't': case 'u': case 'v':
        if (num[from] != '8') return false;
        break;

      case 'w': case 'x': case 'y': case 'z':
        if (num[from] != '9') return false;
        break;

      default:
        return false;
    }

    *to = from + 1;
    return true;
  }

 private:
  char const* const num;
  const int len;
};

int main(int argc, char *argv[]) {
  if (argc != 3) {
    fprintf(stderr, "usage: %s input.index digits\n", argv[0]);
    return 2;
  }

  FILE *fp = fopen(argv[1], "rb");
  if (fp == NULL) {
    fprintf(stderr, "error: can't open \"%s\"\n", argv[1]);
    return 1;
  }

  IndexReader reader(fp);
  PhoneFilter filter(argv[2]);
  SearchDriver driver(&reader, &filter, 0, 1e-6);
  PrintAll(&driver);
  return 0;
}
