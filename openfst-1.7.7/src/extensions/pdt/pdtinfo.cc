// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Prints out various information about a PDT such as number of states, arcs,
// and parentheses.

#include <fst/flags.h>

DEFINE_string(pdt_parentheses, "", "PDT parenthesis label pairs");

int pdtinfo_main(int argc, char **argv);

int main(int argc, char **argv) { return pdtinfo_main(argc, argv); }
