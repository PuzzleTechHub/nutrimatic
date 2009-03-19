// relabel-main.h

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Author: johans@google.com (Johan Schalkwyk)
//
// \file
// Projects a transduction onto its input or output language.
// Includes helper function for fstproject.cc that templates the main
// on the arc type to support multiple and extensible arc types.
//

#ifndef FST_RELABEL_MAIN_H__
#define FST_RELABEL_MAIN_H__

#include <string>
#include <utility>
#include <vector>

#include <fst/relabel.h>
#include <fst/main.h>
#include <fst/text-io.h>
#include <fst/vector-fst.h>

DECLARE_string(relabel_isymbols);
DECLARE_string(relabel_osymbols);
DECLARE_string(relabel_ipairs);
DECLARE_string(relabel_opairs);

DECLARE_bool(allow_negative_labels);  // not recommended; may cause conflicts

namespace fst {

// Main function for fstrelabel templated on the arc type.
// Call only with 'return CALL_FST_MAIN' in main().
template <class Arc>
int RelabelMain(int argc, char **argv, istream &strm,
                const FstReadOptions &opts) {
  Fst<Arc> *ifst = Fst<Arc>::Read(strm, opts);
  if (!ifst) return 1;

  MutableFst<Arc> *ofst = 0;
  if (ifst->Properties(kMutable, false)) {
    ofst = down_cast<MutableFst<Arc> *>(ifst);
  } else {
    ofst = new VectorFst<Arc>(*ifst);
    delete ifst;
  }

  // Relabel with symbol tables
  if (ifst->InputSymbols() || ifst->OutputSymbols()) {
    SymbolTable* isymbols = 0;
    SymbolTable* osymbols = 0;
    if (!FLAGS_relabel_isymbols.empty())
      isymbols = SymbolTable::ReadText(FLAGS_relabel_isymbols,
                                       FLAGS_allow_negative_labels);
    if (!FLAGS_relabel_osymbols.empty())
      osymbols = SymbolTable::ReadText(FLAGS_relabel_osymbols,
                                       FLAGS_allow_negative_labels);

    Relabel(ofst, isymbols, osymbols);
  } else {
    // read in relabel pairs and parse
    typedef typename Arc::Label Label;
    vector<pair<Label, Label> > ipairs;
    vector<pair<Label, Label> > opairs;
    if (!FLAGS_relabel_ipairs.empty())
      ReadLabelPairs(FLAGS_relabel_ipairs, &ipairs,
                     FLAGS_allow_negative_labels);
    if (!FLAGS_relabel_opairs.empty())
      ReadLabelPairs(FLAGS_relabel_opairs, &opairs,
                     FLAGS_allow_negative_labels);
    Relabel(ofst, ipairs, opairs);
  }

  ofst->Write(argc > 2 ? argv[2] : "");

  return 0;
}

}  // namespace fst

#endif  // FST_RELABEL_MAIN_H__
