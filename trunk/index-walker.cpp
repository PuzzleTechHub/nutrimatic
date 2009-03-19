#include "index.h"

#include <assert.h>
#include <limits.h>
#include <stdlib.h>

using namespace std;

IndexWalker::IndexWalker(const IndexReader* r, off_t node, int count):
    reader(r), buf(NULL), buf_alloc(0) {
  stack.resize((stack_size = 1));
  stack[0].next = 0;
  reader->children(node, count, CHAR_MIN, CHAR_MAX, &stack[0].choices);
  next();
}

void IndexWalker::next() {
  while (stack_size > 0 &&
         stack[stack_size - 1].next == stack[stack_size - 1].choices.size()) {
    stack[--stack_size].choices.clear();
  }

  if (stack_size == 0) {
    text = NULL;
    same = 0; 
    count = 0;
    return;
  }

  same = stack_size - 1;

  do {
    if (++stack_size > stack.size()) stack.resize(stack_size);
    State *parent = &stack[stack_size - 2], *child = &stack[stack_size - 1];
    IndexReader::Choice const& choice = parent->choices[parent->next++];

    child->next = 0;
    assert(child->choices.empty());
    count = reader->children(
        choice.next, choice.count, CHAR_MIN, CHAR_MAX, &child->choices);

    if (stack_size - 1 >= buf_alloc)
      buf = (char*) realloc(buf, (buf_alloc = stack_size*2));
    buf[stack_size - 2] = choice.ch;
  } while (count == 0);

  assert(count > 0);
  assert(stack_size - 1 < buf_alloc);
  buf[stack_size - 1] = '\0';
  text = buf;
}
