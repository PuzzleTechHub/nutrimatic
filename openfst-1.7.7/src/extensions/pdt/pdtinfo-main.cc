// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Prints out various information about a PDT such as number of states, arcs,
// and parentheses.

#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <fst/flags.h>
#include <fst/types.h>
#include <fst/log.h>
#include <fst/extensions/pdt/pdtscript.h>
#include <fst/util.h>

DECLARE_string(pdt_parentheses);

int pdtinfo_main(int argc, char **argv) {
  namespace s = fst::script;
  using fst::ReadLabelPairs;
  using fst::script::FstClass;

  std::string usage = "Prints out information about a PDT.\n\n  Usage: ";
  usage += argv[0];
  usage += " in.pdt\n";

  std::set_new_handler(FailedNewHandler);
  SET_FLAGS(usage.c_str(), &argc, &argv, true);
  if (argc > 2) {
    ShowUsage();
    return 1;
  }

  const std::string in_name =
      (argc > 1 && (strcmp(argv[1], "-") != 0)) ? argv[1] : "";

  std::unique_ptr<FstClass> ifst(FstClass::Read(in_name));
  if (!ifst) return 1;

  if (FLAGS_pdt_parentheses.empty()) {
    LOG(ERROR) << argv[0] << ": No PDT parenthesis label pairs provided";
    return 1;
  }

  std::vector<std::pair<int64, int64>> parens;
  if (!ReadLabelPairs(FLAGS_pdt_parentheses, &parens, false)) return 1;

  s::PrintPdtInfo(*ifst, parens);

  return 0;
}
