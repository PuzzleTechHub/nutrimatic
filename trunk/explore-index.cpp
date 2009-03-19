// Walk the nodes in a Nutrimatic index file from a specified starting point,
// descending recursively to the highest-frequency child nodes first.
// Mostly used for debugging and exploring the index contents.

#include "index.h"

#include <assert.h>
#include <limits.h>
#include <string.h>

#include <algorithm>
#include <string>
#include <vector>

using namespace std;

bool by_count(IndexReader::Choice const& a, IndexReader::Choice const& b) {
  return a.count > b.count;
}

static void walk(IndexReader const& reader, off_t node, int count,
                 const char *path, int depth, string *sofar) {
  if (depth == 0) return;

  vector<IndexReader::Choice> children;
  if (*path != '\0') {
    reader.children(node, count, *path, *path, &children);
    ++path;
  } else {
    reader.children(node, count, CHAR_MIN, CHAR_MAX, &children);
  }

  sort(children.begin(), children.end(), by_count);

  for (size_t i = 0; i < children.size(); ++i) {
    sofar->push_back(children[i].ch);
    printf("%s (%d) @%lld\n", sofar->c_str(),
        children[i].count, children[i].next);
    walk(reader, children[i].next, children[i].count, path, depth - 1, sofar);
    sofar->resize(sofar->size() - 1);
  }
}

int main(int argc, char *argv[]) {
  if (argc < 3) {
    fprintf(stderr, "usage: %s input.index \"path\" [depth]\n", argv[0]);
    return 2;
  }

  FILE *input = fopen(argv[1], "rb");
  if (input == NULL) {
    fprintf(stderr, "error: can't open \"%s\"\n", argv[1]);
    return 1;
  }

  IndexReader reader(input);

  printf("Root (%d) @%llu\n", reader.count(), reader.root());

  int depth = strlen(argv[2]);
  if (argc > 3) {
    depth = atoi(argv[3]);
    if (depth == 0) {
      fprintf(stderr, "error: invalid depth \"%s\"\n", argv[3]);
      exit(2);
    }
  }

  string sofar = "";
  walk(reader, reader.root(), reader.count(), argv[2], depth, &sofar);
  return 0;
}
