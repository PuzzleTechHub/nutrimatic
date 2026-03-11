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

/* UTF-8: decode next code point, return it and advance *p. Returns -1 on invalid. */
static int utf8_next(char const** p, char const* end) {
  char const* s = *p;
  if (s >= end) return -1;
  unsigned char b = (unsigned char)*s++;
  int cp;
  if (b < 0x80) {
    cp = b;
  } else if (b >= 0xC2 && b < 0xE0 && s < end) {
    unsigned char b1 = (unsigned char)*s++;
    if ((b1 & 0xC0) != 0x80) return -1;
    cp = ((b & 0x1F) << 6) | (b1 & 0x3F);
  } else if (b >= 0xE0 && b < 0xF0 && s + 1 < end) {
    unsigned char b1 = (unsigned char)*s++;
    unsigned char b2 = (unsigned char)*s++;
    if ((b1 & 0xC0) != 0x80 || (b2 & 0xC0) != 0x80) return -1;
    cp = ((b & 0x0F) << 12) | ((b1 & 0x3F) << 6) | (b2 & 0x3F);
    if (cp < 0x800) return -1;
  } else if (b >= 0xF0 && b < 0xF5 && s + 2 < end) {
    unsigned char b1 = (unsigned char)*s++;
    unsigned char b2 = (unsigned char)*s++;
    unsigned char b3 = (unsigned char)*s++;
    if ((b1 & 0xC0) != 0x80 || (b2 & 0xC0) != 0x80 || (b3 & 0xC0) != 0x80)
      return -1;
    cp = ((b & 0x07) << 18) | ((b1 & 0x3F) << 12) | ((b2 & 0x3F) << 6) | (b3 & 0x3F);
    if (cp < 0x10000) return -1;
  } else {
    return -1;
  }
  *p = s;
  return cp;
}

/* True if code point is a letter or digit we keep (Latin, digit, apostrophe, Cyrillic). */
static bool is_letter(int cp) {
  if (cp < 0) return false;
  if (cp >= '0' && cp <= '9') return true;
  if (cp >= 'a' && cp <= 'z') return true;
  if (cp >= 'A' && cp <= 'Z') return true;
  if (cp == '\'') return true;
  /* Cyrillic: main block and extended */
  if (cp >= 0x0400 && cp <= 0x04FF) return true;
  if (cp >= 0x0500 && cp <= 0x052F) return true;
  return false;
}

/* Lowercase for Latin and Cyrillic. */
static int to_lower_cp(int cp) {
  if (cp >= 'A' && cp <= 'Z') return cp + ('a' - 'A');
  /* Cyrillic uppercase U+0410..U+042F -> U+0430..U+044F */
  if (cp >= 0x0410 && cp <= 0x042F) return cp + (0x0430 - 0x0410);
  /* Cyrillic legacy uppercase U+0400..U+040F -> U+0450..U+045F */
  if (cp >= 0x0400 && cp <= 0x040F) return cp + (0x0450 - 0x0400);
  return cp;
}

/* Append UTF-8 encoding of cp to buf; return number of bytes written (0 if no room). */
static int utf8_append(char* buf, int* buflen, int maxlen, int cp) {
  if (cp < 0) return 0;
  if (cp < 0x80) {
    if (*buflen >= maxlen) return 0;
    buf[(*buflen)++] = (char)(unsigned char)cp;
    return 1;
  }
  if (cp < 0x800) {
    if (*buflen + 2 > maxlen) return 0;
    buf[(*buflen)++] = (char)(unsigned char)(0xC0 | (cp >> 6));
    buf[(*buflen)++] = (char)(unsigned char)(0x80 | (cp & 0x3F));
    return 2;
  }
  if (cp < 0x10000) {
    if (*buflen + 3 > maxlen) return 0;
    buf[(*buflen)++] = (char)(unsigned char)(0xE0 | (cp >> 12));
    buf[(*buflen)++] = (char)(unsigned char)(0x80 | ((cp >> 6) & 0x3F));
    buf[(*buflen)++] = (char)(unsigned char)(0x80 | (cp & 0x3F));
    return 3;
  }
  if (cp < 0x110000) {
    if (*buflen + 4 > maxlen) return 0;
    buf[(*buflen)++] = (char)(unsigned char)(0xF0 | (cp >> 18));
    buf[(*buflen)++] = (char)(unsigned char)(0x80 | ((cp >> 12) & 0x3F));
    buf[(*buflen)++] = (char)(unsigned char)(0x80 | ((cp >> 6) & 0x3F));
    buf[(*buflen)++] = (char)(unsigned char)(0x80 | (cp & 0x3F));
    return 4;
  }
  return 0;
}

static void do_buffer(char *text, int *len, std::vector<string>* out) {
  out->push_back(std::string(text, *len));
  char* space = (char*) memchr(text, ' ', *len);
  if (space == NULL) space = text + *len - 1;
  *len -= space + 1 - text;
  memmove(text, space + 1, *len);
}

static void do_line(char const* line, std::vector<string>* out) {
  char buf[HISTORY_WINDOW_SIZE];
  int buflen = 0;
  char const* end = line + strlen(line);

  while (line < end) {
    if (buflen >= (int)sizeof(buf)) do_buffer(buf, &buflen, out);

    int cp = utf8_next(&line, end);
    if (cp < 0) {
      /* Invalid UTF-8 or end: skip one byte and treat as non-letter */
      if (line < end) ++line;
      if (buflen > 0 && buf[buflen - 1] != ' ') buf[buflen++] = ' ';
      continue;
    }

    if (is_letter(cp)) {
      int lower = to_lower_cp(cp);
      utf8_append(buf, &buflen, (int)sizeof(buf), lower);
    } else if (cp != '\'' && buflen > 0 && buf[buflen - 1] != ' ') {
      buf[buflen++] = ' ';
    }
  }

  while (buflen > 0) do_buffer(buf, &buflen, out);
}

static void write_index(char const* prefix, int num,
                        std::vector<string>* chains) {
  size_t buf_len = strlen(prefix) + 32;
  char filename[buf_len];
  snprintf(filename, buf_len, "%s.%05d.index", prefix, num);
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
  std::vector<string> chains;
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
