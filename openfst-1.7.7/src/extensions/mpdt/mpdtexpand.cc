// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Expands a (bounded-stack) MPDT as an FST.

#include <fst/flags.h>

DEFINE_string(mpdt_parentheses, "",
              "MPDT parenthesis label pairs with assignments");
DEFINE_bool(connect, true, "Trim output?");
DEFINE_bool(keep_parentheses, false, "Keep PDT parentheses in result?");

int mpdtexpand_main(int argc, char **argv);

int main(int argc, char **argv) { return mpdtexpand_main(argc, argv); }
