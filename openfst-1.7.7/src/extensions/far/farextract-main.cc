// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Extracts component FSTs from an finite-state archive.

#include <string>
#include <vector>

#include <fst/flags.h>
#include <fst/extensions/far/farscript.h>
#include <fst/extensions/far/getters.h>

DECLARE_string(filename_prefix);
DECLARE_string(filename_suffix);
DECLARE_int32(generate_filenames);
DECLARE_string(keys);
DECLARE_string(key_separator);
DECLARE_string(range_delimiter);

int farextract_main(int argc, char **argv) {
  namespace s = fst::script;

  std::string usage = "Extracts FSTs from a finite-state archive.\n\n Usage:";
  usage += argv[0];
  usage += " [in1.far in2.far...]\n";

  std::set_new_handler(FailedNewHandler);
  SET_FLAGS(usage.c_str(), &argc, &argv, true);
  s::ExpandArgs(argc, argv, &argc, &argv);

  std::vector<std::string> in_sources;
  for (int i = 1; i < argc; ++i) in_sources.push_back(argv[i]);
  if (in_sources.empty()) in_sources.push_back("");

  const auto arc_type = s::LoadArcTypeFromFar(in_sources[0]);
  if (arc_type.empty()) return 1;

  s::FarExtract(in_sources, arc_type, FLAGS_generate_filenames, FLAGS_keys,
                FLAGS_key_separator, FLAGS_range_delimiter,
                FLAGS_filename_prefix, FLAGS_filename_suffix);

  return 0;
}
