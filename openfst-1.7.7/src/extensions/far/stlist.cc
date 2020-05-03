// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#include <fst/extensions/far/stlist.h>

#include <ios>

#include <fstream>

namespace fst {

bool IsSTList(const std::string &source) {
  std::ifstream strm(source, std::ios_base::in | std::ios_base::binary);
  if (!strm) return false;
  int32 magic_number = 0;
  ReadType(strm, &magic_number);
  return magic_number == kSTListMagicNumber;
}

}  // namespace fst
