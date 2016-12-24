#include "index.h"

#include <assert.h>
#include <sys/mman.h>

#include <algorithm>

using namespace std;

IndexWriter::IndexWriter(FILE *f): fp(f), pos(ftello(fp)) {
  chain.resize((chain_size = 1));
  chain[0].ch = '\0';
  chain[0].count = 0;
}

void IndexWriter::next(const char *text, int same, int64_t count) {
  assert((text == NULL && count == 0 && same == 0) ||
         (text != NULL && count > 0));
  while (text != NULL && same + 1 < int(chain_size) &&
         text[same] == chain[same + 1].ch) ++same;

  while (int(chain_size) - 1 > same) {
    assert(chain_size >= 2);
    Pending *pending = &chain[--chain_size];
    Pending *parent = &chain[chain_size - 1];
    parent->choices.push_back(write(fp, *pending));
    pending->choices.clear();
  }

  assert(chain_size >= 1);
  while (text != NULL && text[chain_size - 1] != '\0') {
    if (++chain_size > chain.size()) chain.resize(chain_size);
    assert(chain[chain_size - 1].choices.empty());
    chain[chain_size - 1].ch = text[chain_size - 2];
    chain[chain_size - 1].count = 0;
  }

  chain[chain_size - 1].count += count;

  if (text == NULL) {
    assert(same == 0 && count == 0 && chain_size == 1);
    write(fp, chain[0]);
    chain.clear();
    assert(ftello(fp) == pos);
  }
}

IndexWriter::Saved IndexWriter::write(FILE* fp, Pending const& in) {
  Saved out;
  out.ch = in.ch;
  out.count = in.count;

  static const off_t none = -1;
  if (in.choices.size() == 0) {
    out.pos = none;
    assert(out.count > 0);
    return out;
  }

  if (in.choices.size() == 1 && in.count == 0 &&
      in.choices[0].ch >= 0x20 &&
      in.choices[0].ch < 0x80 &&
      in.choices[0].pos == pos) {
    fputc(in.choices[0].ch, fp);
    out.pos = ++pos;
    out.count = in.choices[0].count;
    assert(out.count > 0);
    return out;
  }

  int64_t max_count = 0;
  off_t max_offset = 0;
  for (size_t i = 0; i < in.choices.size(); ++i) {
    assert(i == 0 || in.choices[i].ch > in.choices[i - 1].ch);
    assert(in.choices[i].count > 0);
    out.count += in.choices[i].count;
    max_count = max(max_count, in.choices[i].count);
    if (in.choices[i].pos != none)
      max_offset = max(max_offset, max(pos - in.choices[i].pos, 1L));
  }

  int mode;
  if (max_offset == 0 && max_count < 0x100) {
    mode = 0;
    for (size_t i = 0; i < in.choices.size(); ++i) {
      fputc(in.choices[i].ch, fp);
      fputc(in.choices[i].count, fp);
    }
    pos += 2 * in.choices.size();
  } else if (max_offset < 0xFF && max_count < 0x100) {
    mode = 0x80;
    for (size_t i = 0; i < in.choices.size(); ++i) {
      fputc(in.choices[i].ch, fp);
      fputc(in.choices[i].count, fp);
      fputc(in.choices[i].pos == none ? -1 : pos - in.choices[i].pos, fp);
    }
    pos += 3 * in.choices.size();
  } else if (max_offset < 0xFFFF && max_count < 0x100) {
    mode = 0xA0;
    for (size_t i = 0; i < in.choices.size(); ++i) {
      fputc(in.choices[i].ch, fp);
      fputc(in.choices[i].count, fp);
      off_t op = in.choices[i].pos == none ? -1 : pos - in.choices[i].pos;
      fputc(op, fp);
      fputc(op >> 8, fp);
    }
    pos += 4 * in.choices.size();
  } else if (max_offset < 0xFFFF && max_count < 0x10000) {
    mode = 0xC0;
    for (size_t i = 0; i < in.choices.size(); ++i) {
      fputc(in.choices[i].ch, fp);
      fputc(in.choices[i].count, fp);
      fputc(in.choices[i].count >> 8, fp);
      off_t op = in.choices[i].pos == none ? -1 : pos - in.choices[i].pos;
      fputc(op, fp);
      fputc(op >> 8, fp);
    }
    pos += 5 * in.choices.size();
  } else {
    mode = 0xE0;
    for (size_t i = 0; i < in.choices.size(); ++i) {
      fputc(in.choices[i].ch, fp);
      for (int j = 0; j < 8; ++j)
        fputc(in.choices[i].count >> (j * 8), fp);
      off_t op = in.choices[i].pos == none ? -1 : pos - in.choices[i].pos;
      for (int j = 0; j < 8; ++j)
        fputc(op >> (j * 8), fp);
    }
    pos += 17 * in.choices.size();
  }

  assert(in.choices.size() <= 0x100);
  if (in.choices.size() < 0x20) {
    fputc(in.choices.size() + mode, fp);
    pos += 1;
  } else {
    fputc(in.choices.size(), fp);
    fputc(mode, fp);
    pos += 2;
  }

  out.pos = pos;
  assert(out.count > 0);
  return out;
}
