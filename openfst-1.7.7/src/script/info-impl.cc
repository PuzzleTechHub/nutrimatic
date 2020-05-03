// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#include <fst/script/info-impl.h>

namespace fst {

void FstInfo::Info() const {
  std::ostream &ostrm = std::cout;
  const auto old = ostrm.setf(std::ios::left);
  ostrm.width(50);
  ostrm << "fst type" << FstType() << std::endl;
  ostrm.width(50);
  ostrm << "arc type" << ArcType() << std::endl;
  ostrm.width(50);
  ostrm << "input symbol table" << InputSymbols() << std::endl;
  ostrm.width(50);
  ostrm << "output symbol table" << OutputSymbols() << std::endl;
  if (!LongInfo()) {
    ostrm.setf(old);
    return;
  }
  ostrm.width(50);
  ostrm << "# of states" << NumStates() << std::endl;
  ostrm.width(50);
  ostrm << "# of arcs" << NumArcs() << std::endl;
  ostrm.width(50);
  ostrm << "initial state" << Start() << std::endl;
  ostrm.width(50);
  ostrm << "# of final states" << NumFinal() << std::endl;
  ostrm.width(50);
  ostrm << "# of input/output epsilons" << NumEpsilons() << std::endl;
  ostrm.width(50);
  ostrm << "# of input epsilons" << NumInputEpsilons() << std::endl;
  ostrm.width(50);
  ostrm << "# of output epsilons" << NumOutputEpsilons() << std::endl;
  ostrm.width(50);
  ostrm << "input label multiplicity" << InputLabelMultiplicity() << std::endl;
  ostrm.width(50);
  ostrm << "output label multiplicity" << OutputLabelMultiplicity()
        << std::endl;
  ostrm.width(50);
  std::string arc_type = "";
  if (ArcFilterType() == "epsilon")
    arc_type = "epsilon ";
  else if (ArcFilterType() == "iepsilon")
    arc_type = "input-epsilon ";
  else if (ArcFilterType() == "oepsilon")
    arc_type = "output-epsilon ";
  const auto accessible_label = "# of " + arc_type + "accessible states";
  ostrm.width(50);
  ostrm << accessible_label << NumAccessible() << std::endl;
  const auto coaccessible_label = "# of " + arc_type + "coaccessible states";
  ostrm.width(50);
  ostrm << coaccessible_label << NumCoAccessible() << std::endl;
  const auto connected_label = "# of " + arc_type + "connected states";
  ostrm.width(50);
  ostrm << connected_label << NumConnected() << std::endl;
  const auto numcc_label = "# of " + arc_type + "connected components";
  ostrm.width(50);
  ostrm << numcc_label << NumCc() << std::endl;
  const auto numscc_label = "# of " + arc_type + "strongly conn components";
  ostrm.width(50);
  ostrm << numscc_label << NumScc() << std::endl;
  ostrm.width(50);
  ostrm << "input matcher"
        << (InputMatchType() == MATCH_INPUT
                ? 'y'
                : InputMatchType() == MATCH_NONE ? 'n' : '?')
        << std::endl;
  ostrm.width(50);
  ostrm << "output matcher"
        << (OutputMatchType() == MATCH_OUTPUT
                ? 'y'
                : OutputMatchType() == MATCH_NONE ? 'n' : '?')
        << std::endl;
  ostrm.width(50);
  ostrm << "input lookahead" << (InputLookAhead() ? 'y' : 'n') << std::endl;
  ostrm.width(50);
  ostrm << "output lookahead" << (OutputLookAhead() ? 'y' : 'n') << std::endl;
  uint64 prop = 1;
  for (auto i = 0; i < 64; ++i, prop <<= 1) {
    if (prop & kBinaryProperties) {
      char value = 'n';
      if (Properties() & prop) value = 'y';
      ostrm.width(50);
      ostrm << PropertyNames[i] << value << std::endl;
    } else if (prop & kPosTrinaryProperties) {
      char value = '?';
      if (Properties() & prop) {
        value = 'y';
      } else if (Properties() & prop << 1) {
        value = 'n';
      }
      ostrm.width(50);
      ostrm << PropertyNames[i] << value << std::endl;
    }
  }
  ostrm.setf(old);
}

}  // namespace fst
