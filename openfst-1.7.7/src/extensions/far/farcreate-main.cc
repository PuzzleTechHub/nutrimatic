// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Creates a finite-state archive from input FSTs.

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
DECLARE_bool(file_list_input);

int farcreate_main(int argc, char **argv) {
  namespace s = fst::script;

  std::string usage =
      "Creates a finite-state archive from input FSTs.\n\n Usage:";
  usage += argv[0];
  usage += " [in1.fst [[in2.fst ...] out.far]]\n";

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

  const auto arc_type = s::LoadArcTypeFromFst(in_sources[0]);
  if (arc_type.empty()) return 1;

  const auto far_type = s::GetFarType(FLAGS_far_type);

  s::FarCreate(in_sources, out_source, arc_type, FLAGS_generate_keys, far_type,
               FLAGS_key_prefix, FLAGS_key_suffix);

  return 0;
}
