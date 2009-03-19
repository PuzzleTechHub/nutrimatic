// encode-main.h

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
// Encodes labels and/or weights. Used to encode a weight transducer
// into a weightless transducer or acceptor.
//

#ifndef FST_ENCODE_MAIN_H__
#define FST_ENCODE_MAIN_H__

#include <fst/connect.h>
#include <fst/encode.h>
#include <fst/main.h>
#include <fst/vector-fst.h>

DECLARE_bool(encode_labels);
DECLARE_bool(encode_weights);
DECLARE_bool(encode_reuse);
DECLARE_bool(decode);

namespace fst {

// Main function for fstencode templated on the arc type.
// Call only with 'return CALL_FST_MAIN' in main().
template <class Arc>
int EncodeMain(int argc, char **argv, istream &strm,
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

  if (FLAGS_decode == false) {
    uint32 flags = 0;
    flags |= FLAGS_encode_labels ? kEncodeLabels : 0;
    flags |= FLAGS_encode_weights ? kEncodeWeights : 0;
    EncodeMapper<Arc> *encoder = FLAGS_encode_reuse
        ? EncodeMapper<Arc>::Read(argv[2], ENCODE)
        : new EncodeMapper<Arc>(flags, ENCODE);
    Encode(ofst, encoder);
    if (!FLAGS_encode_reuse)
      encoder->Write(argv[2]);
    ofst->Write(argc > 3 ? argv[3] : "");
  } else {
    EncodeMapper<Arc> *decoder = EncodeMapper<Arc>::Read(argv[2], DECODE);
    Decode(ofst, *decoder);
    ofst->Write(argc > 3 ? argv[3] : "");
  }

  delete ofst;
  return 0;
}

}  // namespace fst

#endif  // FST_ENCODE_MAIN_H__
