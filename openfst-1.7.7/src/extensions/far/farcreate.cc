#include <fst/flags.h>

DEFINE_string(key_prefix, "", "Prefix to append to keys");
DEFINE_string(key_suffix, "", "Suffix to append to keys");
DEFINE_int32(generate_keys, 0,
             "Generate N digit numeric keys (def: use file basenames)");
DEFINE_string(far_type, "default",
              "FAR file format type: one of: \"default\", "
              "\"stlist\", \"sttable\"");
DEFINE_bool(file_list_input, false,
            "Each input file contains a list of files to be processed");

int farcreate_main(int argc, char **argv);

int main(int argc, char **argv) {
  return farcreate_main(argc, argv);
}
