// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Composes a PDT and an FST.

#include <fst/flags.h>

DEFINE_string(pdt_parentheses, "", "PDT parenthesis label pairs");
DEFINE_bool(left_pdt, true, "Is the first argument the PDT?");
DEFINE_bool(connect, true, "Trim output?");
DEFINE_string(compose_filter, "paren",
              "Composition filter, one of: \"expand\", \"expand_paren\", "
              "\"paren\"");

int pdtcompose_main(int argc, char **argv);

int main(int argc, char **argv) { return pdtcompose_main(argc, argv); }
