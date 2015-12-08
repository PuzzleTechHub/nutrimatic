// Generates a random FST according to a class-specific transition model
//

#include <cstdlib>
#include <ctime>
#include <string>

#include <fst/extensions/compress/randmod.h>

#include <fst/fstlib.h>
#include <fst/random-weight.h>
#include <fst/util.h>

DEFINE_int32(seed, time(0), "Random seed");
DEFINE_int32(states, 10, "# of states");
DEFINE_int32(labels, 2, "# of labels");
DEFINE_int32(classes, 1, "# of probability distributions");
DEFINE_bool(transducer, false, "Output a transducer");

DEFINE_bool(weights, false, "Output a weighted FST");

int main(int argc, char **argv) {
  using fst::StdVectorFst;
  using fst::StdArc;
  using fst::TropicalWeightGenerator;

  string usage = "Generates a random FST.\n\n  Usage: ";
  usage += argv[0];
  usage += "[out.fst]\n";

  std::set_new_handler(FailedNewHandler);
  SET_FLAGS(usage.c_str(), &argc, &argv, true);
  if (argc > 2) {
    ShowUsage();
    return 1;
  }


  string out_name = (argc > 1 && (strcmp(argv[1], "-") != 0)) ? argv[1] : "";

  srand(FLAGS_seed);

  int num_states = (rand() % FLAGS_states) + 1;     // NOLINT
  int num_classes = (rand() % FLAGS_classes) + 1;   // NOLINT
  int num_labels = (rand() % FLAGS_labels) + 1;     // NOLINT

  StdVectorFst fst;
  TropicalWeightGenerator *weight_gen = 0;
  if (FLAGS_weights)
    weight_gen = new TropicalWeightGenerator(FLAGS_seed, false);
  fst::RandMod<StdArc, TropicalWeightGenerator>
      rand_mod(num_states, num_classes, num_labels, FLAGS_transducer,
               weight_gen);
  rand_mod.Generate(&fst);
  fst.Write(out_name);
  return 0;
}
