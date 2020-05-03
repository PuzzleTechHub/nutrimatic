// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#include <fst/flags.h>

DEFINE_string(arc_type, "standard", "Output arc type");
DEFINE_string(epsilon_symbol, "<eps>", "Epsilon symbol");
DEFINE_string(unknown_symbol, "<unk>", "Unknown word symbol");
DEFINE_string(vocab, "", "Path to the vocabulary file");
DEFINE_string(out, "", "Path to the output binary");
DEFINE_string(save_isymbols, "", "Save input symbol table to file");
DEFINE_string(save_fsymbols, "", "Save feature symbol table to file");
DEFINE_string(save_osymbols, "", "Save output symbol table to file");

int fstlinear_main(int argc, char **argv);

int main(int argc, char **argv) { return fstlinear_main(argc, argv); }
