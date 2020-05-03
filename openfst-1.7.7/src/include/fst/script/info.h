// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#ifndef FST_SCRIPT_INFO_H_
#define FST_SCRIPT_INFO_H_

#include <string>
#include <tuple>

#include <fst/script/arg-packs.h>
#include <fst/script/fst-class.h>
#include <fst/script/info-impl.h>

namespace fst {
namespace script {

using InfoArgs = std::tuple<const FstClass &, bool, const std::string &,
                            const std::string &, bool>;

template <class Arc>
void Info(InfoArgs *args) {
  const Fst<Arc> &fst = *std::get<0>(*args).GetFst<Arc>();
  const FstInfo info(fst, std::get<1>(*args), std::get<2>(*args),
                     std::get<3>(*args), std::get<4>(*args));
  info.Info();
}

void Info(const FstClass &fst, bool test_properties,
          const std::string &arc_filter, const std::string &info_type,
          bool verify);

}  // namespace script
}  // namespace fst

#endif  // FST_SCRIPT_INFO_H_
