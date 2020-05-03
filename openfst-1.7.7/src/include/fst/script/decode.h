// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#ifndef FST_SCRIPT_DECODE_H_
#define FST_SCRIPT_DECODE_H_

#include <tuple>
#include <utility>

#include <fst/encode.h>
#include <fst/script/encodemapper-class.h>
#include <fst/script/fst-class.h>

namespace fst {
namespace script {

using DecodeArgs = std::pair<MutableFstClass *, const EncodeMapperClass &>;

template <class Arc>
void Decode(DecodeArgs *args) {
  MutableFst<Arc> *fst = std::get<0>(*args)->GetMutableFst<Arc>();
  const EncodeMapper<Arc> &mapper = *std::get<1>(*args).GetEncodeMapper<Arc>();
  Decode(fst, mapper);
}

void Decode(MutableFstClass *fst, const EncodeMapperClass &encoder);

}  // namespace script
}  // namespace fst

#endif  // FST_SCRIPT_DECODE_H_
