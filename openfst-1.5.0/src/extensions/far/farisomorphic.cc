// Tests if two Far files contains isomorphic (key,fst) pairs.

#include <fst/extensions/far/main.h>
#include <fst/extensions/far/farscript.h>

DEFINE_string(begin_key, "",
              "First key to extract (def: first key in archive)");
DEFINE_string(end_key, "",
              "Last key to extract (def: last key in archive)");
DEFINE_double(delta, fst::kDelta, "Comparison/quantization delta");

int main(int argc, char **argv) {
  namespace s = fst::script;

  string usage = "Compares the FSTs in two FST archives for isomorphism.";
  usage += "\n\n Usage:";
  usage += argv[0];
  usage += " in1.far in2.far\n";
  usage += "  Flags: begin_key end_key";

  std::set_new_handler(FailedNewHandler);
  SET_FLAGS(usage.c_str(), &argc, &argv, true);

  if (argc != 3) {
    ShowUsage();
    return 1;
  }

  string filename1(argv[1]), filename2(argv[2]);

  bool result = s::FarIsomorphic(
      filename1, filename2, fst::LoadArcTypeFromFar(filename1),
      FLAGS_delta, FLAGS_begin_key, FLAGS_end_key);

  if (!result)
    VLOG(1) << "FARs are not isomorphic.";

  return result ? 0 : 2;
}
