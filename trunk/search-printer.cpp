#include "index.h"
#include "search.h"

#include <stdio.h>

void PrintAll(SearchDriver* d) {
  int count = 0;
  for (;;) {
    if (!(++count % 100000)) {
      printf("# %d\n", count);
      fflush(stdout);
    }
    if (d->step()) {
      if (d->text == NULL) break;
      printf("%g %s\n", d->score, d->text);
    }
  }
}
