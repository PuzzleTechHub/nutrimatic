// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Definitions of 'scriptable' versions of pdt operations, that is,
// those that can be called with FstClass-type arguments.
//
// See comments in nlp/fst/script/script-impl.h for how the registration
// mechanism allows these to work with various arc types.

#include <fst/extensions/pdt/pdtscript.h>

#include <string>
#include <utility>
#include <vector>

#include <fst/types.h>
#include <fst/extensions/pdt/compose.h>
#include <fst/extensions/pdt/expand.h>
#include <fst/extensions/pdt/replace.h>
#include <fst/extensions/pdt/reverse.h>
#include <fst/extensions/pdt/shortest-path.h>
#include <fst/script/script-impl.h>

namespace fst {
namespace script {

void PdtCompose(const FstClass &ifst1, const FstClass &ifst2,
                const std::vector<std::pair<int64, int64>> &parens,
                MutableFstClass *ofst, const PdtComposeOptions &copts,
                bool left_pdt) {
  if (!internal::ArcTypesMatch(ifst1, ifst2, "PdtCompose") ||
      !internal::ArcTypesMatch(ifst1, *ofst, "PdtCompose")) {
    return;
  }
  PdtComposeArgs args(ifst1, ifst2, parens, ofst, copts, left_pdt);
  Apply<Operation<PdtComposeArgs>>("PdtCompose", ifst1.ArcType(), &args);
}

REGISTER_FST_OPERATION_3ARCS(PdtCompose, PdtComposeArgs);

void PdtExpand(const FstClass &ifst,
               const std::vector<std::pair<int64, int64>> &parens,
               MutableFstClass *ofst, const PdtExpandOptions &opts) {
  PdtExpandArgs args(ifst, parens, ofst, opts);
  Apply<Operation<PdtExpandArgs>>("PdtExpand", ifst.ArcType(), &args);
}

REGISTER_FST_OPERATION_3ARCS(PdtExpand, PdtExpandArgs);

void PdtExpand(const FstClass &ifst,
               const std::vector<std::pair<int64, int64>> &parens,
               MutableFstClass *ofst, bool connect, bool keep_parentheses,
               const WeightClass &weight_threshold) {
  PdtExpand(ifst, parens, ofst,
            PdtExpandOptions(connect, keep_parentheses, weight_threshold));
}

void PdtReplace(const std::vector<std::pair<int64, const FstClass *>> &pairs,
                MutableFstClass *ofst,
                std::vector<std::pair<int64, int64>> *parens, int64 root,
                PdtParserType parser_type, int64 start_paren_labels,
                const std::string &left_paren_prefix,
                const std::string &right_paren_prefix) {
  for (size_t i = 1; i < pairs.size(); ++i) {
    if (!internal::ArcTypesMatch(*pairs[i - 1].second, *pairs[i].second,
                                 "PdtReplace"))
      return;
  }
  if (!internal::ArcTypesMatch(*pairs[0].second, *ofst, "PdtReplace")) return;
  PdtReplaceArgs args(pairs, ofst, parens, root, parser_type,
                      start_paren_labels, left_paren_prefix,
                      right_paren_prefix);
  Apply<Operation<PdtReplaceArgs>>("PdtReplace", ofst->ArcType(), &args);
}

REGISTER_FST_OPERATION_3ARCS(PdtReplace, PdtReplaceArgs);

void PdtReverse(const FstClass &ifst,
                const std::vector<std::pair<int64, int64>> &parens,
                MutableFstClass *ofst) {
  PdtReverseArgs args(ifst, parens, ofst);
  Apply<Operation<PdtReverseArgs>>("PdtReverse", ifst.ArcType(), &args);
}

REGISTER_FST_OPERATION_3ARCS(PdtReverse, PdtReverseArgs);

void PdtShortestPath(const FstClass &ifst,
                     const std::vector<std::pair<int64, int64>> &parens,
                     MutableFstClass *ofst,
                     const PdtShortestPathOptions &opts) {
  PdtShortestPathArgs args(ifst, parens, ofst, opts);
  Apply<Operation<PdtShortestPathArgs>>("PdtShortestPath", ifst.ArcType(),
                                        &args);
}

REGISTER_FST_OPERATION_3ARCS(PdtShortestPath, PdtShortestPathArgs);

void PrintPdtInfo(const FstClass &ifst,
                  const std::vector<std::pair<int64, int64>> &parens) {
  PrintPdtInfoArgs args(ifst, parens);
  Apply<Operation<PrintPdtInfoArgs>>("PrintPdtInfo", ifst.ArcType(), &args);
}

REGISTER_FST_OPERATION_3ARCS(PrintPdtInfo, PrintPdtInfoArgs);

}  // namespace script
}  // namespace fst
