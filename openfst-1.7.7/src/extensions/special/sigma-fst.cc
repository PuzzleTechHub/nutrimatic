// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#include <fst/extensions/special/sigma-fst.h>

#include <fst/arc.h>
#include <fst/fst.h>
#include <fst/register.h>

DEFINE_int64(sigma_fst_sigma_label, 0,
             "Label of transitions to be interpreted as sigma ('any') "
             "transitions");
DEFINE_string(sigma_fst_rewrite_mode, "auto",
              "Rewrite both sides when matching? One of:"
              " \"auto\" (rewrite iff acceptor), \"always\", \"never\"");

namespace fst {

const char sigma_fst_type[] = "sigma";
const char input_sigma_fst_type[] = "input_sigma";
const char output_sigma_fst_type[] = "output_sigma";

REGISTER_FST(SigmaFst, StdArc);
REGISTER_FST(SigmaFst, LogArc);
REGISTER_FST(SigmaFst, Log64Arc);

REGISTER_FST(InputSigmaFst, StdArc);
REGISTER_FST(InputSigmaFst, LogArc);
REGISTER_FST(InputSigmaFst, Log64Arc);

REGISTER_FST(OutputSigmaFst, StdArc);
REGISTER_FST(OutputSigmaFst, LogArc);
REGISTER_FST(OutputSigmaFst, Log64Arc);

}  // namespace fst
