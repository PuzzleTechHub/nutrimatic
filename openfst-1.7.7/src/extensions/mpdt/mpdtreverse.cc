// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Reverses an MPDT.

#include <fst/flags.h>

DEFINE_string(mpdt_parentheses, "",
              "MPDT parenthesis label pairs with assignments.");
DEFINE_string(mpdt_new_parentheses, "",
              "Output for reassigned parentheses and stacks");

int mpdtreverse_main(int argc, char **argv);

int main(int argc, char **argv) { return mpdtreverse_main(argc, argv); }
