// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#include <fst/flags.h>
#include <fst/log.h>
#include <fst/extensions/linear/linear-fst.h>
#include <fst/extensions/linear/loglinear-apply.h>
#include <fst/vector-fst.h>

DECLARE_bool(normalize);

int fstloglinearapply_main(int argc, char **argv) {
  std::string usage =
      "Applies an FST to another FST, treating the second as a log-linear "
      "model.\n\n  "
      "Usage: ";
  usage += argv[0];
  usage += " in.fst linear.fst [out.fst]\n";

  std::set_new_handler(FailedNewHandler);
  SET_FLAGS(usage.c_str(), &argc, &argv, true);
  if (argc < 3 || argc > 4) {
    ShowUsage();
    return 1;
  }

  std::string in_name = strcmp(argv[1], "-") != 0 ? argv[1] : "";
  std::string linear_name =
      (argc > 2 && (strcmp(argv[2], "-") != 0)) ? argv[2] : "";
  std::string out_name =
      (argc > 3 && (strcmp(argv[3], "-") != 0)) ? argv[3] : "";

  if (in_name.empty() && linear_name.empty()) {
    LOG(ERROR) << argv[0] << ": Can't take both inputs from standard input.";
    return 1;
  }

  fst::StdFst *ifst1 = fst::StdFst::Read(in_name);
  if (!ifst1) return 1;

  fst::StdFst *ifst2 = fst::StdFst::Read(linear_name);
  if (!ifst2) return 1;

  fst::StdVectorFst ofst;

  fst::LogLinearApply(*ifst1, *ifst2, &ofst, FLAGS_normalize);

  return !ofst.Write(out_name);
}
