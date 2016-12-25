#include "index.h"

#include <algorithm>
#include <string>
#include <vector>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace std;

static const size_t CHAINS_PER_FILE = 1000000;
static const size_t MAX_LINE_LENGTH = 65536;
static const size_t HISTORY_WINDOW_SIZE = 40;
static const size_t TITLE_MULTIPLIER = 10;

static void do_buffer(char *text, int *len, vector<string>* out) {
  out->push_back(string(text, *len));
  char* space = (char*) memchr(text, ' ', *len);
  if (space == NULL) space = text + *len - 1;
  *len -= space + 1 - text;
  memmove(text, space + 1, *len);
}

static void do_line(char const* line, vector<string>* out) {
  char buf[HISTORY_WINDOW_SIZE];
  int buflen = 0;

  for (; *line != '\0'; ++line) {
    if (buflen == sizeof(buf)) do_buffer(buf, &buflen, out);

    if (isalnum(*line)) {
      buf[buflen++] = tolower(*line);
    } else if (*line != '\'' && buflen > 0 && buf[buflen - 1] != ' ') {
      buf[buflen++] = ' ';
    }
  }

  while (buflen > 0) do_buffer(buf, &buflen, out);
}

static void write_index(char const* prefix, int num, vector<string>* chains) {
  char filename[strlen(prefix) + 32];
  sprintf(filename, "%s.%05d.index", prefix, num);
  FILE *fp = fopen(filename, "w");
  if (fp == NULL) {
    fprintf(stderr, "error: can't open \"%s\"\n", filename);
    exit(1);
  }

  IndexWriter writer(fp);
  sort(chains->begin(), chains->end());
  for (size_t i = 0; i < chains->size(); ++i) {
    int same = 0;
    if (i > 0) {
      int len = min((*chains)[i - 1].size(), (*chains)[i].size());
      while (same < len && (*chains)[i - 1][same] == (*chains)[i][same]) ++same;
    }
    writer.next((*chains)[i].c_str(), same, 1);
  }

  writer.next(NULL, 0, 0);
  chains->clear();
  fclose(fp);
}

int main(int argc, char* argv[]) {
  if (argc != 2 || argv[1][0] == '-') {
    fprintf(stderr, "usage: %s outfileprefix < textfile.txt\n", argv[0]);
    return 2;
  }

  int filecount = 0;
  char buf[MAX_LINE_LENGTH];
  vector<string> chains;
  bool next_line_is_title = false;
  while (fgets(buf, sizeof(buf), stdin)) {
    // Handle both output from remove-markup (with BEGIN ARTICLE: and
    // END ARTICLE: lines) and WikiExtractor.py (with <doc ...> and </doc>).
    if (!strncmp(buf, "BEGIN ARTICLE:", 14)) {
      for (size_t i = 0; i < TITLE_MULTIPLIER; ++i) do_line(buf + 14, &chains);
    } else if (!strncmp(buf, "<doc ", 5)) {
      next_line_is_title = true;
    } else if (next_line_is_title) {
      for (size_t i = 0; i < TITLE_MULTIPLIER; ++i) do_line(buf, &chains);
      next_line_is_title = false;
    } else if (strncmp(buf, "END ARTICLE:", 12) && strncmp(buf, "</doc>", 6)) {
      do_line(buf, &chains);
    }

    if (chains.size() >= CHAINS_PER_FILE)
      write_index(argv[1], filecount++, &chains);
  }

  if (chains.size() > 0) write_index(argv[1], filecount++, &chains);
  return 0;
}
