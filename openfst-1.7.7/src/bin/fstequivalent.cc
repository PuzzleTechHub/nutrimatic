#include <limits>
#include <random>

#include <fst/flags.h>
#include <fst/weight.h>

DEFINE_double(delta, fst::kDelta, "Comparison/quantization delta");
DEFINE_bool(random, false,
            "Test equivalence by randomly selecting paths in the input FSTs");
DEFINE_int32(max_length, std::numeric_limits<int32>::max(),
             "Maximum path length");
DEFINE_int32(npath, 1, "Number of paths to generate");
DEFINE_uint64(seed, std::random_device()(), "Random seed");
DEFINE_string(select, "uniform",
              "Selection type: one of: "
              " \"uniform\", \"log_prob\" (when appropriate),"
              " \"fast_log_prob\" (when appropriate)");

int fstequivalent_main(int argc, char **argv);

int main(int argc, char **argv) { return fstequivalent_main(argc, argv); }
