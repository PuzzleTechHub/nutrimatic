// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#ifndef FST_SCRIPT_CONCAT_H_
#define FST_SCRIPT_CONCAT_H_

#include <utility>

#include <fst/concat.h>
#include <fst/script/fst-class.h>

namespace fst {
namespace script {

using ConcatArgs1 = std::pair<MutableFstClass *, const FstClass &>;

template <class Arc>
void Concat(ConcatArgs1 *args) {
  MutableFst<Arc> *fst1 = std::get<0>(*args)->GetMutableFst<Arc>();
  const Fst<Arc> &fst2 = *std::get<1>(*args).GetFst<Arc>();
  Concat(fst1, fst2);
}

using ConcatArgs2 = std::pair<const FstClass &, MutableFstClass *>;

template <class Arc>
void Concat(ConcatArgs2 *args) {
  const Fst<Arc> &fst1 = *std::get<0>(*args).GetFst<Arc>();
  MutableFst<Arc> *fst2 = std::get<1>(*args)->GetMutableFst<Arc>();
  Concat(fst1, fst2);
}

using ConcatArgs3 =
    std::pair<const std::vector<FstClass *> &, MutableFstClass *>;

template <class Arc>
void Concat(ConcatArgs3 *args) {
  const auto &untyped_fsts1 = std::get<0>(*args);
  std::vector<const Fst<Arc> *> typed_fsts1;
  typed_fsts1.reserve(untyped_fsts1.size());
  for (const auto &untyped_fst1 : untyped_fsts1) {
    typed_fsts1.emplace_back(untyped_fst1->GetFst<Arc>());
  }
  MutableFst<Arc> *fst2 = std::get<1>(*args)->GetMutableFst<Arc>();
  Concat(typed_fsts1, fst2);
}

void Concat(MutableFstClass *fst1, const FstClass &fst2);

void Concat(const FstClass &fst1, MutableFstClass *fst2);

void Concat(const std::vector<FstClass *> &fsts1, MutableFstClass *fst2);

}  // namespace script
}  // namespace fst

#endif  // FST_SCRIPT_CONCAT_H_
