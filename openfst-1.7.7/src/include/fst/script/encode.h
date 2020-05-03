// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#ifndef FST_SCRIPT_ENCODE_H_
#define FST_SCRIPT_ENCODE_H_

#include <tuple>
#include <utility>

#include <fst/encode.h>
#include <fst/script/encodemapper-class.h>
#include <fst/script/fst-class.h>

namespace fst {
namespace script {

using EncodeArgs = std::tuple<MutableFstClass *, EncodeMapperClass *>;

template <class Arc>
void Encode(EncodeArgs *args) {
  MutableFst<Arc> *fst = std::get<0>(*args)->GetMutableFst<Arc>();
  EncodeMapper<Arc> *mapper = std::get<1>(*args)->GetEncodeMapper<Arc>();
  Encode(fst, mapper);
}

void Encode(MutableFstClass *fst, EncodeMapperClass *mapper);

}  // namespace script
}  // namespace fst

#endif  // FST_SCRIPT_ENCODE_H_
