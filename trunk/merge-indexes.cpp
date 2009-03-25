#include "index.h"

#include <algorithm>
#include <queue>
#include <string>
#include <vector>
#include <utility>

#include <assert.h>
#include <stdio.h>
#include <string.h>

using namespace std;

#define DEBUG 0

struct ReaderCompare {
  // True if x should come *after* y.
  bool operator()(IndexWalker* x, IndexWalker* y) const {
    int same = min(x->same, y->same);
    assert(memcmp(x->text, y->text, same) == 0);
    return strcmp(x->text + same, y->text + same) > 0;
  }
};

struct FrequencyCutoffWriter {
  FrequencyCutoffWriter(IndexWriter* out, int min):
      output(out), cutoff(min), output_same(0) {
    words.push_back(make_pair(0, 0));
  }

  void next(const char *text, int same, int count) {
    if (text != NULL) {
      while (same < int(saved.size()) && text[same] == saved[same]) ++same;
      assert(memcmp(saved.c_str(), text, same) == 0);
      assert(strcmp(saved.c_str() + same, text + same) <= 0);
#if DEBUG
      fprintf(stderr, "input: [%.*s|%s] * %d\n", same, text, text+same, count);
#endif
    }

    assert(!words.empty());
    while (words.back().first > (size_t) same) {
      pair<size_t, int> last_word = words.back();
      words.pop_back();

      assert(saved.size() >= last_word.first);
      saved.resize(last_word.first);
      output_same = min(output_same, saved.size());
      if (last_word.second >= cutoff ||
          (last_word.second > 0 && output_same == last_word.first)) {
#if DEBUG
        fprintf(stderr, "output: [%.*s|%s] * %d\n",
            output_same, saved.c_str(),
            saved.c_str()+output_same, last_word.second);
#endif
        output->next(saved.c_str(), output_same, last_word.second);
        output_same = words.back().first;
      } else {
        words.back().second += last_word.second;
        output_same = min(output_same, words.back().first);
      }
    }

    saved.resize(same);
    if (text != NULL) {
      saved.append(text + same);
      while (char *space = strchr(text + same, ' ')) {
        same = space - text + 1;
        words.push_back(make_pair(same, 0));
      }
    }

    if (!words.empty()) words.back().second += count;
    if (text == NULL) output->next(NULL, 0, 0);
  }

 private:
  IndexWriter* const output;
  const int cutoff;
  size_t output_same;
  string saved;
  vector<pair<size_t, int> > words;
};

int main(int argc, char *argv[]) {
  if (argc < 4) {
    fprintf(stderr, "usage: %s min input.index ... out.index\n", argv[0]);
    return 2;
  }

  int cutoff = atoi(argv[1]);
  if (cutoff <= 0) {
    fprintf(stderr, "error: illegal frequency threshold \"%s\"\n", argv[1]);
    return 2;
  }

  priority_queue<IndexWalker*, vector<IndexWalker*>, ReaderCompare> queue;
  for (int i = 2; i < argc - 1; ++i) {
    FILE *fp = fopen(argv[i], "r");
    if (fp == NULL) {
      fprintf(stderr, "error: can't read \"%s\"\n", argv[i]);
      return 1;
    }

    IndexReader* index = new IndexReader(fp);
    IndexWalker* walker = new IndexWalker(index, index->root(), index->count());
    if (walker->text == NULL) {
      fprintf(stderr, "warning: empty input \"%s\"\n", argv[i]);
      delete walker;
    } else {
      queue.push(walker);
    }
  }

  if (fopen(argv[argc - 1], "rb") != NULL) {
    fprintf(stderr, "error: output \"%s\" already exists\n", argv[argc - 1]);
    return 1;
  }

  FILE *out = fopen(argv[argc - 1], "wb");
  if (out == NULL) {
    fprintf(stderr, "error: can't write \"%s\"\n", argv[argc - 1]);
    return 1;
  }
  IndexWriter output(out);
  FrequencyCutoffWriter writer(&output, cutoff);

  while (!queue.empty()) {
    IndexWalker *next = queue.top(); queue.pop();
    writer.next(next->text, next->same, next->count);
    next->next();
    if (next->text == NULL)
      delete next;
    else
      queue.push(next);
  }

  writer.next(NULL, 0, 0);
  return 0;
}
