// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#include <fst/script/info.h>

#include <fst/script/script-impl.h>

namespace fst {
namespace script {

void Info(const FstClass &fst, bool test_properties,
          const std::string &arc_filter, const std::string &info_type,
          bool verify) {
  InfoArgs args(fst, test_properties, arc_filter, info_type, verify);
  Apply<Operation<InfoArgs>>("Info", fst.ArcType(), &args);
}

REGISTER_FST_OPERATION_3ARCS(Info, InfoArgs);

}  // namespace script
}  // namespace fst
