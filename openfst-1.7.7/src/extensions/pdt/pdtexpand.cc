// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Expands a (bounded-stack) PDT as an FST.

#include <fst/flags.h>

DEFINE_string(pdt_parentheses, "", "PDT parenthesis label pairs");
DEFINE_bool(connect, true, "Trim output?");
DEFINE_bool(keep_parentheses, false, "Keep PDT parentheses in result?");
DEFINE_string(weight, "", "Weight threshold");

int pdtexpand_main(int argc, char **argv);

int main(int argc, char **argv) { return pdtexpand_main(argc, argv); }
