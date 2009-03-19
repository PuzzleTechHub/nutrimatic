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

namespace {
  struct CompareReader {
    // True if x should come *after* y.
    bool operator()(IndexWalker* x, IndexWalker* y) const {
      int same = min(x->same, y->same);
      assert(memcmp(x->text, y->text, same) == 0);
      return strcmp(x->text + same, y->text + same) > 0;
    }
  };

  struct WordFilterWriter {
    WordFilterWriter(IndexWriter* out, int min):
      output(out), cutoff(min), output_same(0) { }

    void next(const char *text, int same, int count) {
#if DEBUG
      fprintf(stderr, "input: [%.*s|%s] * %d\n", same, text, text+same, count);
#endif
      if (text != NULL) {
        while (same < int(saved.size()) && text[same] == saved[same]) ++same;
        assert(memcmp(saved.c_str(), text, same) == 0);
        assert(strcmp(saved.c_str() + same, text + same) <= 0);
      }

      while (!spaces.empty() && spaces.back().first >= (size_t) same) {
        assert(saved.size() > spaces.back().first);
        saved.resize(spaces.back().first + 1);
        output_same = min(output_same, saved.size());
        if (spaces.back().second >= cutoff) {
#if DEBUG
          fprintf(stderr, "output: [%.*s|%s] * %d\n",
	      output_same, saved.c_str(),
	      saved.c_str()+output_same, spaces.back().second);
#endif
          output->next(saved.c_str(), output_same, spaces.back().second);
        }
        spaces.resize(spaces.size() - 1);
      }

      saved.resize(same);
      if (text != NULL) {
        saved.append(text + same);
        while (char *space = strchr(text + same, ' ')) {
          spaces.push_back(make_pair(space - text, 0));
          same = space - text + 1;
        }
      }

      if (!spaces.empty()) spaces.back().second += count;
      if (text == NULL) output->next(NULL, 0, 0);
    }

   private:
    IndexWriter* const output;
    const int cutoff;
    size_t output_same;
    string saved;
    vector<pair<size_t, int> > spaces;
  };

  struct FilterWriter {
    FilterWriter(IndexWriter* out, int min):
      output(out), cutoff(min),
      pending_size(0), pending_pos(0), pending_total(0) { }

    void next(const char* text, int same, int count) {
      if (text != NULL) {
        while (same < int(saved.size()) && text[same] == saved[same]) ++same;
        assert(memcmp(saved.c_str(), text, same) == 0);
        assert(strcmp(saved.c_str() + same, text + same) <= 0);
      }

#if DEBUG
      fprintf(stderr, "input: [%.*s|%s] * %d\n", same, text, text+same, count);
#endif

      if (pending_pos >= same) {
        if (pending_total > 0) {
          saved.resize(pending_pos + 1, '\0');
          output->next(saved.c_str(), pending_pos, pending_total);
#if DEBUG
          fprintf(stderr, "  complete: [%.*s|%.*s>%s] * %d\n", same, text,
              pending_pos - same, saved.c_str() + same,
              saved.c_str() + pending_pos, pending_total);
#endif
          pending_total = 0;
        }
        pending_pos = same;
      } else if (same < int(saved.size())) {
        int total = 0;
        for (int i = same + 1; i < pending_size; ++i) {
          for (size_t j = 0; j < pending[i].size(); ++j) {
            total += pending[i][j].second;
          }
        }
        pending[same].push_back(make_pair(saved[same], total));

#if DEBUG
        fprintf(stderr, "  collapse: [%.*s>%.*s] * %d\n", same, text,
            1, saved.c_str() + same, total);
#endif
      }

      saved.resize(same);
      if (text != NULL) saved.append(text + same);

      while (pending_size > same + 1) pending[--pending_size].clear();
      if (count > 0) {
        pending_size = saved.size() + 1;
        if (pending_size > int(pending.size())) pending.resize(pending_size);
        pending[saved.size()].push_back(make_pair('\0', count));
        pending_total += count;
      }

      string tmp;
      for (int pos = pending_pos; pending_total >= cutoff; ++pos) {
        assert(pos < pending_size);
        tmp.assign(saved,  0, pos);

#if DEBUG
        fprintf(stderr, "  suffices: [%s] * %d\n", tmp.c_str(), pending_total);
#endif

        tmp.resize(pos + 1);
        for (size_t i = 0; i < pending[pos].size(); ++i) {
          tmp[pos] = pending[pos][i].first;
          output->next(tmp.c_str(), pending_pos, pending[pos][i].second);
          pending_pos = pos;
          pending_total -= pending[pos][i].second;

#if DEBUG
          fprintf(stderr, "  released: [%.*s>%s] * %d\n", pos, tmp.c_str(),
              tmp.c_str() + pos, pending[pos][i].second);
#endif
        }
        pending[pos].clear();
      }

      if (text == NULL) output->next(NULL, 0, 0);
    }

   private:
    IndexWriter* const output;
    const int cutoff;
    string saved;
    vector<vector<pair<char, int> > > pending;
    int pending_size, pending_pos, pending_total;
  };
}

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

  priority_queue<IndexWalker*, vector<IndexWalker*>, CompareReader> queue;
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
  // FilterWriter writer(&output, cutoff);
  WordFilterWriter writer(&output, cutoff);

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
