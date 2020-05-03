#include <limits>
#include <random>

#include <fst/flags.h>

DEFINE_int32(max_length, std::numeric_limits<int32>::max(),
             "Maximum path length");
DEFINE_int32(npath, 1, "Number of paths to generate");
DEFINE_uint64(seed, std::random_device()(), "Random seed");
DEFINE_string(select, "uniform",
              "Selection type: one of: "
              " \"uniform\", \"log_prob\" (when appropriate),"
              " \"fast_log_prob\" (when appropriate)");
DEFINE_bool(weighted, false,
            "Output tree weighted by path count vs. unweighted paths");
DEFINE_bool(remove_total_weight, false,
            "Remove total weight when output weighted");

int fstrandgen_main(int argc, char **argv);

int main(int argc, char **argv) { return fstrandgen_main(argc, argv); }
