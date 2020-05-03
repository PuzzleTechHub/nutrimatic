// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Compiles a set of stings as FSTs and stores them in a finite-state archive.

#include <string>
#include <vector>

#include <fst/flags.h>
#include <fst/extensions/far/farscript.h>
#include <fst/extensions/far/getters.h>
#include <fstream>

DECLARE_string(key_prefix);
DECLARE_string(key_suffix);
DECLARE_int32(generate_keys);
DECLARE_string(far_type);
DECLARE_bool(allow_negative_labels);
DECLARE_string(arc_type);
DECLARE_string(entry_type);
DECLARE_string(fst_type);
DECLARE_string(token_type);
DECLARE_string(symbols);
DECLARE_string(unknown_symbol);
DECLARE_bool(file_list_input);
DECLARE_bool(keep_symbols);
DECLARE_bool(initial_symbols);

int farcompilestrings_main(int argc, char **argv) {
  namespace s = fst::script;

  std::string usage = "Compiles a set of strings as FSTs and stores them in";
  usage += " a finite-state archive.\n\n  Usage:";
  usage += argv[0];
  usage += " [in1.txt [[in2.txt ...] out.far]]\n";

  std::set_new_handler(FailedNewHandler);
  SET_FLAGS(usage.c_str(), &argc, &argv, true);
  s::ExpandArgs(argc, argv, &argc, &argv);

  std::vector<std::string> in_sources;
  if (FLAGS_file_list_input) {
    for (int i = 1; i < argc - 1; ++i) {
      std::ifstream istrm(argv[i]);
      std::string str;
      while (getline(istrm, str)) in_sources.push_back(str);
    }
  } else {
    for (int i = 1; i < argc - 1; ++i)
      in_sources.push_back(strcmp(argv[i], "-") != 0 ? argv[i] : "");
  }
  if (in_sources.empty()) {
    // argc == 1 || argc == 2.  This cleverly handles both the no-file case
    // and the one (input) file case together.
    // TODO(jrosenstock): This probably shouldn't happen for the
    // --file_list_input case.
    in_sources.push_back(argc == 2 && strcmp(argv[1], "-") != 0 ? argv[1] : "");
  }

  // argc <= 2 means the file (if any) is an input file, so write to stdout.
  const std::string out_source =
      argc > 2 && strcmp(argv[argc - 1], "-") != 0 ? argv[argc - 1] : "";

  fst::FarEntryType entry_type;
  if (!s::GetFarEntryType(FLAGS_entry_type, &entry_type)) {
    LOG(ERROR) << "Unknown or unsupported FAR entry type: " << FLAGS_entry_type;
    return 1;
  }

  fst::FarTokenType token_type;
  if (!s::GetFarTokenType(FLAGS_token_type, &token_type)) {
    LOG(ERROR) << "Unkonwn or unsupported FAR token type: " << FLAGS_token_type;
    return 1;
  }

  const auto far_type = s::GetFarType(FLAGS_far_type);

  s::FarCompileStrings(in_sources, out_source, FLAGS_arc_type, FLAGS_fst_type,
                       far_type, FLAGS_generate_keys, entry_type, token_type,
                       FLAGS_symbols, FLAGS_unknown_symbol, FLAGS_keep_symbols,
                       FLAGS_initial_symbols, FLAGS_allow_negative_labels,
                       FLAGS_key_prefix, FLAGS_key_suffix);

  return 0;
}
