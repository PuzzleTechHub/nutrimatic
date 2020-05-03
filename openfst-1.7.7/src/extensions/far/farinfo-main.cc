// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Prints some basic information about the FSTs in an FST archive.

#include <string>
#include <vector>

#include <fst/flags.h>
#include <fst/extensions/far/farscript.h>
#include <fst/extensions/far/getters.h>

DECLARE_string(begin_key);
DECLARE_string(end_key);
DECLARE_bool(list_fsts);

int farinfo_main(int argc, char **argv) {
  namespace s = fst::script;

  std::string usage = "Prints some basic information about the FSTs in an FST ";
  usage += "archive.\n\n  Usage:";
  usage += argv[0];
  usage += " [in1.far in2.far...]\n";
  usage += "  Flags: begin_key end_key list_fsts";

  std::set_new_handler(FailedNewHandler);
  SET_FLAGS(usage.c_str(), &argc, &argv, true);
  s::ExpandArgs(argc, argv, &argc, &argv);

  std::vector<std::string> in_sources;
  for (int i = 1; i < argc; ++i) in_sources.push_back(argv[i]);
  if (in_sources.empty()) in_sources.push_back("");

  const auto arc_type = s::LoadArcTypeFromFar(in_sources[0]);
  if (arc_type.empty()) return 1;

  s::FarInfo(in_sources, arc_type, FLAGS_begin_key, FLAGS_end_key,
             FLAGS_list_fsts);

  return 0;
}
