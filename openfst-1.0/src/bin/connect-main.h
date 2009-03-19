// connect-main.h

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
// Removes useless (inaccessible or non-coaccessible) states and arcs
// from an FST. Includes helper function for fstconnect.cc that
// templates the main on the arc type to support multiple and
// extensible arc types.
//

#ifndef FST_CONNECT_MAIN_H__
#define FST_CONNECT_MAIN_H__

#include <fst/connect.h>
#include <fst/main.h>
#include <fst/vector-fst.h>

namespace fst {

// Main function for fstconnect templated on the arc type.
// Call only with 'return CALL_FST_MAIN' in main().
template <class Arc>
int ConnectMain(int argc, char **argv, istream &strm,
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

  Connect(ofst);
  ofst->Write(argc > 2 ? argv[2] : "");

  return 0;
}

}  // namespace fst

#endif  // FST_CONNECT_MAIN_H__
