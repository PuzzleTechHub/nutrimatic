// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Returns the shortest path in a (bounded-stack) PDT.

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

DECLARE_bool(keep_parentheses);
DECLARE_string(queue_type);
DECLARE_bool(path_gc);
DECLARE_string(pdt_parentheses);

int pdtshortestpath_main(int argc, char **argv) {
  namespace s = fst::script;
  using fst::script::FstClass;
  using fst::script::VectorFstClass;
  using fst::QueueType;
  using fst::ReadLabelPairs;

  std::string usage = "Shortest path in a (bounded-stack) PDT.\n\n  Usage: ";
  usage += argv[0];
  usage += " in.pdt [out.fst]\n";

  std::set_new_handler(FailedNewHandler);
  SET_FLAGS(usage.c_str(), &argc, &argv, true);
  if (argc > 3) {
    ShowUsage();
    return 1;
  }

  const std::string in_name =
      (argc > 1 && (strcmp(argv[1], "-") != 0)) ? argv[1] : "";
  const std::string out_name =
      (argc > 2 && (strcmp(argv[2], "-") != 0)) ? argv[2] : "";

  std::unique_ptr<FstClass> ifst(FstClass::Read(in_name));
  if (!ifst) return 1;

  if (FLAGS_pdt_parentheses.empty()) {
    LOG(ERROR) << argv[0] << ": No PDT parenthesis label pairs provided";
    return 1;
  }

  std::vector<std::pair<int64, int64>> parens;
  if (!ReadLabelPairs(FLAGS_pdt_parentheses, &parens, false)) return 1;

  VectorFstClass ofst(ifst->ArcType());

  QueueType qt;
  if (FLAGS_queue_type == "fifo") {
    qt = fst::FIFO_QUEUE;
  } else if (FLAGS_queue_type == "lifo") {
    qt = fst::LIFO_QUEUE;
  } else if (FLAGS_queue_type == "state") {
    qt = fst::STATE_ORDER_QUEUE;
  } else {
    LOG(ERROR) << "Unknown queue type: " << FLAGS_queue_type;
    return 1;
  }

  const s::PdtShortestPathOptions opts(qt, FLAGS_keep_parentheses,
                                       FLAGS_path_gc);

  s::PdtShortestPath(*ifst, parens, &ofst, opts);

  return !ofst.Write(out_name);
}
