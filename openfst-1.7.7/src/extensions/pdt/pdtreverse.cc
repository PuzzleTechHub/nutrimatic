// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Reverses a PDT.

#include <fst/flags.h>

DEFINE_string(pdt_parentheses, "", "PDT parenthesis label pairs");

int pdtreverse_main(int argc, char **argv);

int main(int argc, char **argv) { return pdtreverse_main(argc, argv); }
