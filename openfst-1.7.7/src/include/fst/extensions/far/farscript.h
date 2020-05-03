// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Convenience file for including all of the FAR operations, or registering
// them for new arc types.

#ifndef FST_EXTENSIONS_FAR_FARSCRIPT_H_
#define FST_EXTENSIONS_FAR_FARSCRIPT_H_

#include <string>
#include <vector>

#include <fst/types.h>
#include <fst/extensions/far/compile-strings.h>
#include <fst/extensions/far/create.h>
#include <fst/extensions/far/equal.h>
#include <fst/extensions/far/extract.h>
#include <fst/extensions/far/far-class.h>
#include <fst/extensions/far/far.h>
#include <fst/extensions/far/info.h>
#include <fst/extensions/far/isomorphic.h>
#include <fst/extensions/far/print-strings.h>
#include <fst/extensions/far/script-impl.h>
#include <fst/script/arg-packs.h>

namespace fst {
namespace script {

// Note: it is safe to pass these strings as references because this struct is
// only used to pass them deeper in the call graph. Be sure you understand why
// this is so before using this struct for anything else!
struct FarCompileStringsArgs {
  const std::vector<std::string> &in_sources;
  const std::string &out_source;
  const std::string &fst_type;
  const FarType &far_type;
  const int32 generate_keys;
  const FarEntryType fet;
  const FarTokenType tt;
  const std::string &symbols_source;
  const std::string &unknown_symbol;
  const bool keep_symbols;
  const bool initial_symbols;
  const bool allow_negative_labels;
  const std::string &key_prefix;
  const std::string &key_suffix;

  FarCompileStringsArgs(const std::vector<std::string> &in_sources,
                        const std::string &out_source,
                        const std::string &fst_type, const FarType &far_type,
                        int32 generate_keys, FarEntryType fet, FarTokenType tt,
                        const std::string &symbols_source,
                        const std::string &unknown_symbol, bool keep_symbols,
                        bool initial_symbols, bool allow_negative_labels,
                        const std::string &key_prefix,
                        const std::string &key_suffix)
      : in_sources(in_sources),
        out_source(out_source),
        fst_type(fst_type),
        far_type(far_type),
        generate_keys(generate_keys),
        fet(fet),
        tt(tt),
        symbols_source(symbols_source),
        unknown_symbol(unknown_symbol),
        keep_symbols(keep_symbols),
        initial_symbols(initial_symbols),
        allow_negative_labels(allow_negative_labels),
        key_prefix(key_prefix),
        key_suffix(key_suffix) {}
};

template <class Arc>
void FarCompileStrings(FarCompileStringsArgs *args) {
  FarCompileStrings<Arc>(
      args->in_sources, args->out_source, args->fst_type, args->far_type,
      args->generate_keys, args->fet, args->tt, args->symbols_source,
      args->unknown_symbol, args->keep_symbols, args->initial_symbols,
      args->allow_negative_labels, args->key_prefix, args->key_suffix);
}

void FarCompileStrings(const std::vector<std::string> &in_sources,
                       const std::string &out_source,
                       const std::string &arc_type, const std::string &fst_type,
                       const FarType &far_type, int32 generate_keys,
                       FarEntryType fet, FarTokenType tt,
                       const std::string &symbols_source,
                       const std::string &unknown_symbol, bool keep_symbols,
                       bool initial_symbols, bool allow_negative_labels,
                       const std::string &key_prefix,
                       const std::string &key_suffix);

// Note: it is safe to pass these strings as references because this struct is
// only used to pass them deeper in the call graph. Be sure you understand why
// this is so before using this struct for anything else!
struct FarCreateArgs {
  const std::vector<std::string> &in_sources;
  const std::string &out_source;
  const int32 generate_keys;
  const FarType &far_type;
  const std::string &key_prefix;
  const std::string &key_suffix;

  FarCreateArgs(const std::vector<std::string> &in_sources,
                const std::string &out_source, const int32 generate_keys,
                const FarType &far_type, const std::string &key_prefix,
                const std::string &key_suffix)
      : in_sources(in_sources),
        out_source(out_source),
        generate_keys(generate_keys),
        far_type(far_type),
        key_prefix(key_prefix),
        key_suffix(key_suffix) {}
};

template <class Arc>
void FarCreate(FarCreateArgs *args) {
  FarCreate<Arc>(args->in_sources, args->out_source, args->generate_keys,
                 args->far_type, args->key_prefix, args->key_suffix);
}

void FarCreate(const std::vector<std::string> &in_sources,
               const std::string &out_source, const std::string &arc_type,
               const int32 generate_keys, const FarType &far_type,
               const std::string &key_prefix, const std::string &key_suffix);

using FarEqualInnerArgs =
    std::tuple<const std::string &, const std::string &, float,
               const std::string &, const std::string &>;

using FarEqualArgs = WithReturnValue<bool, FarEqualInnerArgs>;

template <class Arc>
void FarEqual(FarEqualArgs *args) {
  args->retval = fst::FarEqual<Arc>(
      std::get<0>(args->args), std::get<1>(args->args), std::get<2>(args->args),
      std::get<3>(args->args), std::get<4>(args->args));
}

bool FarEqual(const std::string &source1, const std::string &source2,
              const std::string &arc_type, float delta = kDelta,
              const std::string &begin_key = std::string(),
              const std::string &end_key = std::string());

using FarExtractArgs =
    std::tuple<const std::vector<std::string> &, int32, const std::string &,
               const std::string &, const std::string &, const std::string &,
               const std::string &>;

template <class Arc>
void FarExtract(FarExtractArgs *args) {
  fst::FarExtract<Arc>(std::get<0>(*args), std::get<1>(*args),
                           std::get<2>(*args), std::get<3>(*args),
                           std::get<4>(*args), std::get<5>(*args),
                           std::get<6>(*args));
}

void FarExtract(const std::vector<std::string> &isources,
                const std::string &arc_type, int32 generate_sources,
                const std::string &keys, const std::string &key_separator,
                const std::string &range_delimiter,
                const std::string &source_prefix,
                const std::string &source_suffix);

using FarInfoArgs =
    std::tuple<const std::vector<std::string> &, const std::string &,
               const std::string &, const bool>;

template <class Arc>
void FarInfo(FarInfoArgs *args) {
  fst::FarInfo<Arc>(std::get<0>(*args), std::get<1>(*args),
                        std::get<2>(*args), std::get<3>(*args));
}

void FarInfo(const std::vector<std::string> &sources,
             const std::string &arc_type, const std::string &begin_key,
             const std::string &end_key, const bool list_fsts);

using GetFarInfoArgs =
    std::tuple<const std::vector<std::string> &, const std::string &,
               const std::string &, const bool, FarInfoData *>;

template <class Arc>
void GetFarInfo(GetFarInfoArgs *args) {
  fst::GetFarInfo<Arc>(std::get<0>(*args), std::get<1>(*args),
                           std::get<2>(*args), std::get<3>(*args),
                           std::get<4>(*args));
}

void GetFarInfo(const std::vector<std::string> &sources,
                const std::string &arc_type, const std::string &begin_key,
                const std::string &end_key, const bool list_fsts,
                FarInfoData *);

using FarIsomorphicInnerArgs =
    std::tuple<const std::string &, const std::string &, float,
               const std::string &, const std::string &>;

using FarIsomorphicArgs = WithReturnValue<bool, FarIsomorphicInnerArgs>;

template <class Arc>
void FarIsomorphic(FarIsomorphicArgs *args) {
  args->retval = fst::FarIsomorphic<Arc>(
      std::get<0>(args->args), std::get<1>(args->args), std::get<2>(args->args),
      std::get<3>(args->args), std::get<4>(args->args));
}

bool FarIsomorphic(const std::string &source1, const std::string &source2,
                   const std::string &arc_type, float delta = kDelta,
                   const std::string &begin_key = std::string(),
                   const std::string &end_key = std::string());

struct FarPrintStringsArgs {
  const std::vector<std::string> &isources;
  const FarEntryType entry_type;
  const FarTokenType token_type;
  const std::string &begin_key;
  const std::string &end_key;
  const bool print_key;
  const bool print_weight;
  const std::string &symbols_source;
  const bool initial_symbols;
  const int32 generate_sources;
  const std::string &source_prefix;
  const std::string &source_suffix;

  FarPrintStringsArgs(const std::vector<std::string> &isources,
                      const FarEntryType entry_type,
                      const FarTokenType token_type,
                      const std::string &begin_key, const std::string &end_key,
                      const bool print_key, const bool print_weight,
                      const std::string &symbols_source,
                      const bool initial_symbols, const int32 generate_sources,
                      const std::string &source_prefix,
                      const std::string &source_suffix)
      : isources(isources),
        entry_type(entry_type),
        token_type(token_type),
        begin_key(begin_key),
        end_key(end_key),
        print_key(print_key),
        print_weight(print_weight),
        symbols_source(symbols_source),
        initial_symbols(initial_symbols),
        generate_sources(generate_sources),
        source_prefix(source_prefix),
        source_suffix(source_suffix) {}
};

template <class Arc>
void FarPrintStrings(FarPrintStringsArgs *args) {
  fst::FarPrintStrings<Arc>(
      args->isources, args->entry_type, args->token_type, args->begin_key,
      args->end_key, args->print_key, args->print_weight, args->symbols_source,
      args->initial_symbols, args->generate_sources, args->source_prefix,
      args->source_suffix);
}

void FarPrintStrings(const std::vector<std::string> &isources,
                     const std::string &arc_type, const FarEntryType entry_type,
                     const FarTokenType token_type,
                     const std::string &begin_key, const std::string &end_key,
                     const bool print_key, const bool print_weight,
                     const std::string &symbols_source,
                     const bool initial_symbols, const int32 generate_sources,
                     const std::string &source_prefix,
                     const std::string &source_suffix);

}  // namespace script
}  // namespace fst

#define REGISTER_FST_FAR_OPERATIONS(ArcType)                                 \
  REGISTER_FST_OPERATION(FarCompileStrings, ArcType, FarCompileStringsArgs); \
  REGISTER_FST_OPERATION(FarCreate, ArcType, FarCreateArgs);                 \
  REGISTER_FST_OPERATION(FarEqual, ArcType, FarEqualArgs);                   \
  REGISTER_FST_OPERATION(FarExtract, ArcType, FarExtractArgs);               \
  REGISTER_FST_OPERATION(FarInfo, ArcType, FarInfoArgs);                     \
  REGISTER_FST_OPERATION(FarIsomorphic, ArcType, FarIsomorphicArgs);         \
  REGISTER_FST_OPERATION(FarPrintStrings, ArcType, FarPrintStringsArgs);     \
  REGISTER_FST_OPERATION(GetFarInfo, ArcType, GetFarInfoArgs)

#endif  // FST_EXTENSIONS_FAR_FARSCRIPT_H_
