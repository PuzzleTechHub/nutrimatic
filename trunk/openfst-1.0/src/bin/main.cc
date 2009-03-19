// main.cc

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
// Author: riley@google.com (Michael Riley)
//
// \file
// Classes and functions for registering and invoking Fst main
// functions that support multiple and extensible arc types.

#include <iostream>

#include <iostream>
#include <fstream>
#include <fst/main.h>

// General main flag definitions.
DEFINE_bool(acceptor, false, "Input in acceptor format");
DEFINE_string(arc_filter, "any", "Arc filter: one of :\
 \"any\", \"epsilon\", \"iepsilon\", \"oepsilon\"");
DEFINE_string(arc_type, "standard", "Output arc type");
DEFINE_bool(connect, true, "Trim output");
DEFINE_double(delta, fst::kDelta, "Comparison/quantization delta");
DEFINE_string(fst_type, "vector", "Output FST type");
DEFINE_string(isymbols, "", "Input label symbol table");
DEFINE_string(osymbols, "", "Output label symbol table");
DEFINE_bool(reverse, false, "Perform in the reverse direction");
DEFINE_string(ssymbols, "", "State label symbol table");
DEFINE_bool(to_final, false, "Push/reweight to final (vs. to initial) states");
DEFINE_string(weight, "", "Weight parameter");
DEFINE_int64(nstate, fst::kNoStateId, "State number parameter");
DEFINE_bool(allow_negative_labels, false,
            "Allow negative labels (not recommended; may cause conflicts)");

// ArcsortMain specific flag definitions.
DEFINE_string(sort_type, "ilabel",
              "Comparison method, one of: \"ilabel\", \"olabel\"");

// ClosureMain specific flag definitions.
DEFINE_bool(closure_plus, false,
            "Do not add the empty path (T+ instead of T*)");

// CompileMain specific flag definitions.
DEFINE_bool(keep_isymbols, false, "Store input label symbol table with FST");
DEFINE_bool(keep_osymbols, false, "Store output label symbol table with FST");
DEFINE_bool(keep_state_numbering, false, "Do not renumber input states");

// ComposeMain specific flag definitions.
DEFINE_string(compose_filter, "sequence", "Compose filter");

// DrawMain specific flag definitions.
DEFINE_int32(fontsize, 14, "Set fontsize");
DEFINE_double(height, 11, "Set height");
DEFINE_double(width, 8.5, "Set width");
DEFINE_bool(portrait, false, "Portrait mode (def: landscape)");
DEFINE_bool(vertical, false, "Draw bottom-to-top instead of left-to-right");
DEFINE_string(title, "", "Set figure title");
DEFINE_int32(precision, 5, "Set precision (number of char/float)");
DEFINE_double(nodesep, 0.25,
              "Set minimum separation between nodes (see dot documentation)");
DEFINE_double(ranksep, 0.40,
              "Set minimum separation between ranks (see dot documentation)");

// InfoMain specific flag definitions.
DEFINE_bool(pipe, false, "Send info to stderr, input to stdout");
DEFINE_bool(test_properties, true,
            "Compute property values (if unknown to FST)");

// PrintMain and DrawMain specific flag definitions.
DEFINE_bool(numeric, false, "Print numeric labels");
DEFINE_string(save_isymbols, "", "Save input symbol table to file");
DEFINE_string(save_osymbols, "", "Save output symbol table to file");
DEFINE_bool(show_weight_one, false,
            "Print/draw arc weights and final weights equal to Weight::One()");

// ProjectMain specific flag definitions.
DEFINE_bool(project_output, false, "Project on output (vs. input)");

// EquivalentMain specific flag definitions
DEFINE_bool(random, false,
            "Test equivalence by randomly selecting paths in the input FSTs");

/// EncodeMain specific flag definitions
DEFINE_bool(encode_labels, false, "Encode output labels");
DEFINE_bool(encode_weights, false, "Encode weights");
DEFINE_bool(encode_reuse, false, "Re-use existing codex");
DEFINE_bool(decode, false, "Decode labels and/or weights");

// EpsNormalizeMain specific flag definitions
DEFINE_bool(eps_norm_output, false, "Normalize output epsilons");

// MapMain specific flag definitions.
DEFINE_string(map_type, "identity",
              "Map operation, one of: \"identity\", \"invert\",\
 \"plus (--weight)\", \"quantize (--delta)\", \"rmweight\",\
 \"superfinal\", \"times (--weight)\"\n");

// RelabelMain specific flag definitions
// If the Fst has symbol sets then you need to specify the relabel
// symbol sets.
DEFINE_string(relabel_isymbols, "", "Input symbol set to relabel to");
DEFINE_string(relabel_osymbols, "", "Ouput symbol set to relabel to");

// If the Fst does not have symbol sets you need to specify the
// numeric Label pairs.
DEFINE_string(relabel_ipairs, "", "Input relabel pairs (numeric)");
DEFINE_string(relabel_opairs, "", "Output relabel pairs (numeric)");

// PushMain specific flag definitions.
DEFINE_bool(push_weights, false, "Push weights");
DEFINE_bool(push_labels, false, "Push output labels");

// RandgenMain specific flag definitions.
DEFINE_int32(max_length, INT_MAX, "Maximum path length");
DEFINE_int32(npath, 1, "Number of paths to generate");
DEFINE_int32(seed, time(0), "Random seed");
DEFINE_string(select, "uniform", "Selection type: one of:\
 \"uniform\", \"log_prob (when appropriate)\"");

// ShortestPathMain specific flag definitions.
DEFINE_int64(nshortest, 1, "Return N-shortest paths");
DEFINE_bool(unique, false, "Return unique strings");

// ReplaceMain specific flag definitions
DEFINE_bool(epsilon_on_replace, false, "Create an espilon arc when recursing");

// DeterminizeMain specific flag definitions
DEFINE_int64(subsequential_label, 0,
             "Input label of arc corresponding to residual final output when\
 producing a subsequential transducer");

namespace fst {

FstOnceType FstMainRegister::register_init_ = FST_ONCE_INIT;

Mutex *FstMainRegister::register_lock_ = 0;

FstMainRegister *FstMainRegister::register_ = 0;

// Invokes mfunc<Arc>. If atype == "", arc type is determined from Fst
// at argv[1].
int CallFstMain(const string &mtype, int argc, char **argv, string atype) {
  const char *filename = "standard input";
  istream *strm = &std::cin;
  FstReadOptions opts;
  FstHeader hdr;
  if (atype == "") {
    if (argc > 1 && strcmp(argv[1], "-") != 0) {
      filename = argv[1];
      strm = new ifstream(filename);
      if (!*strm) {
        LOG(ERROR) << argv[0] << ": Can't open file: " << filename;
        return 1;
      }
    }
    if (!hdr.Read(*strm, filename))
      return 1;
    atype = hdr.ArcType();
    opts.source = filename;
    opts.header = &hdr;
  }

  FstMainRegister *registr = FstMainRegister::GetRegister();
  const FstMainRegister::Main main = registr->GetMain(mtype, atype);
  if (!main) {
    LOG(ERROR) << argv[0] << ": Bad or unknown arc type \"" << atype
               << "\" for this operation (" << mtype << ")";
    return 1;
  }
  int ret = main(argc, argv, *strm, opts);
  if (strm != &std::cin)
    delete strm;
  return ret;
}

}  // namespace fst
