// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#include <fst/extensions/special/rho-fst.h>

#include <fst/arc.h>
#include <fst/fst.h>
#include <fst/register.h>

DEFINE_int64(rho_fst_rho_label, 0,
             "Label of transitions to be interpreted as rho ('rest') "
             "transitions");
DEFINE_string(rho_fst_rewrite_mode, "auto",
              "Rewrite both sides when matching? One of:"
              " \"auto\" (rewrite iff acceptor), \"always\", \"never\"");

namespace fst {

const char rho_fst_type[] = "rho";
const char input_rho_fst_type[] = "input_rho";
const char output_rho_fst_type[] = "output_rho";

REGISTER_FST(RhoFst, StdArc);
REGISTER_FST(RhoFst, LogArc);
REGISTER_FST(RhoFst, Log64Arc);

REGISTER_FST(InputRhoFst, StdArc);
REGISTER_FST(InputRhoFst, LogArc);
REGISTER_FST(InputRhoFst, Log64Arc);

REGISTER_FST(OutputRhoFst, StdArc);
REGISTER_FST(OutputRhoFst, LogArc);
REGISTER_FST(OutputRhoFst, Log64Arc);

}  // namespace fst
