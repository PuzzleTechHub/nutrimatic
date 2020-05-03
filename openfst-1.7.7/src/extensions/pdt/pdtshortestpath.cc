// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Returns the shortest path in a (bounded-stack) PDT.

#include <fst/flags.h>

DEFINE_bool(keep_parentheses, false, "Keep PDT parentheses in result?");
DEFINE_string(queue_type, "fifo",
              "Queue type: one of: "
              "\"fifo\", \"lifo\", \"state\"");
DEFINE_bool(path_gc, true, "Garbage collect shortest path data?");
DEFINE_string(pdt_parentheses, "", "PDT parenthesis label pairs");

int pdtshortestpath_main(int argc, char **argv);

int main(int argc, char **argv) { return pdtshortestpath_main(argc, argv); }
