#include <fst/flags.h>

DEFINE_string(filename_prefix, "", "Prefix to append to filenames");
DEFINE_string(filename_suffix, "", "Suffix to append to filenames");
DEFINE_int32(generate_filenames, 0,
             "Generate N digit numeric filenames (def: use keys)");
DEFINE_string(keys, "",
              "Extract set of keys separated by comma (default) "
              "including ranges delimited by dash (default)");
DEFINE_string(key_separator, ",", "Separator for individual keys");
DEFINE_string(range_delimiter, "-", "Delimiter for ranges of keys");

int farextract_main(int argc, char **argv);

int main(int argc, char **argv) {
  return farextract_main(argc, argv);
}
