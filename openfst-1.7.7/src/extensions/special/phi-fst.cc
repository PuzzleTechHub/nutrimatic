// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#include <fst/extensions/special/phi-fst.h>

#include <fst/arc.h>
#include <fst/fst.h>
#include <fst/register.h>

DEFINE_int64(phi_fst_phi_label, 0,
             "Label of transitions to be interpreted as phi ('failure') "
              "transitions");
DEFINE_bool(phi_fst_phi_loop, true,
            "When true, a phi self loop consumes a symbol");
DEFINE_string(phi_fst_rewrite_mode, "auto",
              "Rewrite both sides when matching? One of:"
              " \"auto\" (rewrite iff acceptor), \"always\", \"never\"");

namespace fst {

const char phi_fst_type[] = "phi";
const char input_phi_fst_type[] = "input_phi";
const char output_phi_fst_type[] = "output_phi";

REGISTER_FST(PhiFst, StdArc);
REGISTER_FST(PhiFst, LogArc);
REGISTER_FST(PhiFst, Log64Arc);

REGISTER_FST(InputPhiFst, StdArc);
REGISTER_FST(InputPhiFst, LogArc);
REGISTER_FST(InputPhiFst, Log64Arc);

REGISTER_FST(OutputPhiFst, StdArc);
REGISTER_FST(OutputPhiFst, LogArc);
REGISTER_FST(OutputPhiFst, Log64Arc);

}  // namespace fst
