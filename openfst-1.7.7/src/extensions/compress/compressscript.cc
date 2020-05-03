// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#include <fst/extensions/compress/compressscript.h>

#include <fst/arc-map.h>
#include <fst/script/script-impl.h>

namespace fst {
namespace script {

bool Compress(const FstClass &fst, const std::string &source) {
  CompressInnerArgs iargs(fst, source);
  CompressArgs args(iargs);
  Apply<Operation<CompressArgs>>("Compress", fst.ArcType(), &args);
  return args.retval;
}

REGISTER_FST_OPERATION_3ARCS(Compress, CompressArgs);

bool Decompress(const std::string &source, MutableFstClass *fst) {
  DecompressInnerArgs iargs(source, fst);
  DecompressArgs args(iargs);
  Apply<Operation<DecompressArgs>>("Decompress", fst->ArcType(), &args);
  return args.retval;
}

REGISTER_FST_OPERATION_3ARCS(Decompress, DecompressArgs);

}  // namespace script
}  // namespace fst
