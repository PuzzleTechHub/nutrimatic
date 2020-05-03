// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#include <fst/flags.h>
#include <fst/extensions/linear/linearscript.h>

DECLARE_string(arc_type);
DECLARE_string(epsilon_symbol);
DECLARE_string(unknown_symbol);
DECLARE_string(vocab);
DECLARE_string(out);
DECLARE_string(save_isymbols);
DECLARE_string(save_fsymbols);
DECLARE_string(save_osymbols);

int fstlinear_main(int argc, char **argv) {
  // TODO(wuke): more detailed usage
  std::set_new_handler(FailedNewHandler);
  SET_FLAGS(argv[0], &argc, &argv, true);
  fst::script::ValidateDelimiter();
  fst::script::ValidateEmptySymbol();

  if (argc == 1) {
    ShowUsage();
    return 1;
  }

  fst::script::LinearCompile(FLAGS_arc_type, FLAGS_epsilon_symbol,
                                 FLAGS_unknown_symbol, FLAGS_vocab, argv + 1,
                                 argc - 1, FLAGS_out, FLAGS_save_isymbols,
                                 FLAGS_save_fsymbols, FLAGS_save_osymbols);

  return 0;
}
