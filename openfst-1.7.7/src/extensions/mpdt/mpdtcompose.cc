// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Composes an MPDT and an FST.

#include <fst/flags.h>

DEFINE_string(mpdt_parentheses, "",
              "MPDT parenthesis label pairs with assignments");
DEFINE_bool(left_mpdt, true, "Is the first argument the MPDT?");
DEFINE_bool(connect, true, "Trim output?");
DEFINE_string(compose_filter, "paren",
              "Composition filter, one of: \"expand\", \"expand_paren\", "
              "\"paren\"");

int mpdtcompose_main(int argc, char **argv);

int main(int argc, char **argv) { return mpdtcompose_main(argc, argv); }
