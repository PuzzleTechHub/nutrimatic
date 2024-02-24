// Dump a Nutrimatic index file in alphabetic order with frequencies.
// Mainly used for debugging index generation.

#include "index.h"

#include <inttypes.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "usage: %s input.index\n", argv[0]);
    return 2;
  }

  FILE *fp = fopen(argv[1], "r");
  if (fp == NULL) {
    fprintf(stderr, "error: can't open \"%s\"\n", argv[1]);
    return 1;
  }

  IndexReader reader(fp);
  IndexWalker walker(&reader, reader.root(), reader.count());
  while (walker.text != NULL) {
    printf("%5" PRId64 " [%s]\n", walker.count, walker.text);
    walker.next();
  }

  return 0;
}
