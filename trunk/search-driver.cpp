#include "index.h"
#include "search.h"

#include <assert.h>
#include <limits.h>

#include <algorithm>

using namespace std;

SearchDriver::SearchDriver(const IndexReader* r,
                           const SearchFilter* f,
                           SearchFilter::State start,
                           double rp):
    reader(r), filter(f), restart(rp) {
  Next seed;
  seed.crumb = -1;
  seed.scale = 1.0;
  seed.choice.ch = '\0';
  seed.choice.next = reader->root();
  seed.choice.count = reader->count();
  seed.state = start;
  nexts.push(seed);

  text = NULL;
  score = 0;
}

bool SearchDriver::step() {
  if (nexts.empty()) {
    text = NULL;
    score = 0;
    return true;
  }

  const Next next = nexts.top(); nexts.pop();

  Next new_next;
  new_next.crumb = crumbs.size();
  new_next.scale = next.scale;

  tmp.clear();
  reader->children(next.choice.next, next.choice.count, CHAR_MIN, CHAR_MAX, &tmp);
  for (size_t i = 0; i < tmp.size(); ++i) {
    assert(tmp[i].count > 0);
    if (filter->has_transition(next.state, tmp[i].ch, &new_next.state)) {
      if (int(crumbs.size()) == new_next.crumb) {
        Crumb new_crumb;
        new_crumb.parent = next.crumb;
        new_crumb.ch = next.choice.ch;
        crumbs.push_back(new_crumb);
      }
      new_next.choice = tmp[i];
      nexts.push(new_next);
    }
  }

  if (filter->is_accepting(next.state) && next.crumb != -1) {
    size_t len = 0;
    for (int i = next.crumb; i >= 0; i = crumbs[i].parent)
      ++len;

    string buffer(len--, next.choice.ch);
    for (int i = next.crumb; i >= 0 && len > 0; i = crumbs[i].parent)
      buffer[--len] = crumbs[i].ch;
    assert(len == 0);

    pair<set<string>::iterator, bool> ib = seen.insert(buffer);
    if (ib.second) {
      text = ib.first->c_str();
      score = next.scale * next.choice.count;
      return true;
    }
  }

  if (restart > 0.0 &&
      next.choice.ch == ' ' &&
      next.choice.next != reader->root()) {
    new_next.crumb = next.crumb;
    new_next.scale = next.scale * next.choice.count / reader->count() * restart;
    new_next.choice.ch = next.choice.ch;
    new_next.choice.count = reader->count();
    new_next.choice.next = reader->root();
    new_next.state = next.state;
    nexts.push(new_next);
  }

  return false;
}
