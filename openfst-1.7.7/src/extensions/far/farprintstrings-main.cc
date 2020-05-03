// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Outputs as strings the string FSTs in a finite-state archive.

#include <string>
#include <vector>

#include <fst/flags.h>
#include <fst/extensions/far/farscript.h>
#include <fst/extensions/far/getters.h>

DECLARE_string(filename_prefix);
DECLARE_string(filename_suffix);
DECLARE_int32(generate_filenames);
DECLARE_string(begin_key);
DECLARE_string(end_key);
DECLARE_bool(print_key);
DECLARE_bool(print_weight);
DECLARE_string(entry_type);
DECLARE_string(token_type);
DECLARE_string(symbols);
DECLARE_bool(initial_symbols);

int farprintstrings_main(int argc, char **argv) {
  namespace s = fst::script;

  std::string usage =
      "Print as std::string the std::string FSTs in an archive.\n\n  Usage:";
  usage += argv[0];
  usage += " [in1.far in2.far ...]\n";

  std::set_new_handler(FailedNewHandler);
  SET_FLAGS(usage.c_str(), &argc, &argv, true);
  s::ExpandArgs(argc, argv, &argc, &argv);

  std::vector<std::string> in_sources;
  for (int i = 1; i < argc; ++i) in_sources.push_back(argv[i]);
  if (in_sources.empty()) in_sources.push_back("");

  const auto arc_type = s::LoadArcTypeFromFar(in_sources[0]);
  if (arc_type.empty()) return 1;

  fst::FarEntryType entry_type;
  if (!s::GetFarEntryType(FLAGS_entry_type, &entry_type)) {
    LOG(ERROR) << "Unknown or unsupported FAR entry type: " << FLAGS_entry_type;
    return 1;
  }

  fst::FarTokenType token_type;
  if (!s::GetFarTokenType(FLAGS_token_type, &token_type)) {
    LOG(ERROR) << "Unknown or unsupported FAR token type: " << FLAGS_token_type;
    return 1;
  }

  s::FarPrintStrings(in_sources, arc_type, entry_type, token_type,
                     FLAGS_begin_key, FLAGS_end_key, FLAGS_print_key,
                     FLAGS_print_weight, FLAGS_symbols, FLAGS_initial_symbols,
                     FLAGS_generate_filenames, FLAGS_filename_prefix,
                     FLAGS_filename_suffix);

  return 0;
}
