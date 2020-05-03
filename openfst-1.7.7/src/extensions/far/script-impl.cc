// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Definitions and functions for invoking and using Far main functions that
// support multiple and extensible arc types.

#include <fst/extensions/far/script-impl.h>

#include <string>

#include <fst/extensions/far/far.h>
#include <fstream>

namespace fst {
namespace script {

std::string LoadArcTypeFromFar(const std::string &far_source) {
  FarHeader hdr;
  if (!hdr.Read(far_source)) {
    LOG(ERROR) << "Error reading FAR: " << far_source;
    return "";
  }
  std::string atype = hdr.ArcType();
  if (atype == "unknown") {
    LOG(ERROR) << "Empty FST archive: " << far_source;
    return "";
  }
  return atype;
}

std::string LoadArcTypeFromFst(const std::string &fst_source) {
  FstHeader hdr;
  std::ifstream in(fst_source, std::ios_base::in | std::ios_base::binary);
  if (!hdr.Read(in, fst_source)) {
    LOG(ERROR) << "Error reading FST: " << fst_source;
    return "";
  }
  return hdr.ArcType();
}

}  // namespace script
}  // namespace fst
