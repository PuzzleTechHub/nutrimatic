// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#include <fst/compat.h>
#include <fst/flags.h>

DEFINE_bool(normalize, true, "Normalize to get posterior");

int fstloglinearapply_main(int argc, char **argv);

int main(int argc, char **argv) { return fstloglinearapply_main(argc, argv); }
