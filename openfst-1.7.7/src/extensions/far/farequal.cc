#include <fst/flags.h>
#include <fst/weight.h>

DEFINE_string(begin_key, "",
              "First key to extract (def: first key in archive)");
DEFINE_string(end_key, "", "Last key to extract (def: last key in archive)");
DEFINE_double(delta, fst::kDelta, "Comparison/quantization delta");

int farequal_main(int argc, char **argv);

int main(int argc, char **argv) {
  return farequal_main(argc, argv);
}
