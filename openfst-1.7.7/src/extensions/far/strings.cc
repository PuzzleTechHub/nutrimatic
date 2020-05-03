// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#include <cmath>
#include <string>

#include <fst/flags.h>
#include <fst/extensions/far/compile-strings.h>
#include <fstream>

DEFINE_string(far_field_separator, "\t",
              "Set of characters used as a separator between printed fields");

namespace fst {

// Computes the minimal length required to encode each line number as a decimal
// number, or zero if the number of lines could not be determined because the
// file was not seekable.
int KeySize(const char *source) {
  std::ifstream istrm(source);
  istrm.seekg(0);
  // TODO(jrosenstock): Change this to is_regular_file when <filesystem> is
  // no longer banned.
  // Stream not seekable. This is really a hack to approximate is_regular_file.
  // What we really want is that opening and reading the file twice gives the
  // same result, which is only true for regular files. There may be devices
  // that don't return an error on seek. At least we are able to catch the
  // common cases of /dev/stdin and fifos.
  if (istrm.rdstate() & std::ios_base::failbit) {
    return 0;
  }
  std::string s;
  int nline = 0;
  while (getline(istrm, s)) ++nline;
  return nline ? ceil(log10(nline + 1)) : 1;
}

}  // namespace fst
