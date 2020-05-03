#include <fst/flags.h>

DEFINE_string(begin_key, "",
              "First key to extract (default: first key in archive)");
DEFINE_string(end_key, "",
              "Last key to extract (default: last key in archive)");

DEFINE_bool(list_fsts, false, "Display FST information for each key");

int farinfo_main(int argc, char **argv);

int main(int argc, char **argv) {
  return farinfo_main(argc, argv);
}
