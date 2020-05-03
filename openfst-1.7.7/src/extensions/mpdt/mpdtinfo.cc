// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Prints out various information about an MPDT such as number of states, arcs,
// and parentheses.

#include <fst/flags.h>

DEFINE_string(mpdt_parentheses, "",
              "MPDT parenthesis label pairs with assignments");

int mpdtinfo_main(int argc, char **argv);

int main(int argc, char **argv) { return mpdtinfo_main(argc, argv); }
