#define _FILE_OFFSET_BITS 64
#include <stdio.h>
#include <unistd.h>

#include <queue>
#include <vector>

/*
  Index format: The index is series of trie nodes, parents following children.
  Each trie node is a table of letter, frequency, and child-node-offset values.
  For space efficiency, there are several node formats:

  Parent of leaves (no child has children) with byte-sized frequency values:

    (letter frequency)* (num[01..1F] | num 00)

  One child node, immediately preceding, with the same frequency as this node:

    letter[20-7F]

  Byte-sized frequency and offset values:

    (letter frequency offset)* (num[01..1F]+80 | num 80)

  Byte-sized frequency, 2-byte offset values:

    (letter frequency offset:2)* (num[01..1F]+A0 | num A0)

  2-byte frequency and 2-byte offset values:

    (letter frequency:2 offset:2)* (num[01..1F]+C0 | num C0)

  4-byte frequency and 8-byte offset values:

    (letter frequency:4 offset:8)* (num[01..1F]+E0 | num E0)

  In all cases, offset values are from the end of the child node to the
  start of the parent node.  An offset of 0 means the child immediately
  precedes the parent node.  The maximum offset (all FF) means there is
  no child node (NULL pointer equivalent).
*/

class IndexWriter {
 public:
  IndexWriter(FILE*);
  void next(const char* text, int same, int count);

 private:
  FILE* const fp;
  off_t pos;

  struct Saved { int ch; int count; off_t pos; };
  struct Pending { int ch; int count; std::vector<Saved> choices; };
  std::vector<Pending> chain;
  size_t chain_size;

  Saved write(FILE* fp, Pending const&);
};

class IndexReader {
 public:
  IndexReader(FILE*);
  ~IndexReader();

  typedef off_t Node;
  Node root() const { return length; }
  int count() const { return total; }

  struct Choice { char ch; int count; Node next; };
  int children(Node parent, int count,
               char min, char max,
               std::vector<Choice>* out) const;

 private:
  const unsigned char* data;
  ssize_t length;
  int total;
  void fail(off_t n, const char* message) const;
};

class IndexWalker {
 public:
  const char* text;
  int same, count;

  IndexWalker(const IndexReader*, IndexReader::Node node, int count);
  void next();

 private:
  const IndexReader* const reader;
  char* buf;

  struct State { std::vector<IndexReader::Choice> choices; size_t next; };
  std::vector<State> stack;
  size_t stack_size, buf_alloc;
};
