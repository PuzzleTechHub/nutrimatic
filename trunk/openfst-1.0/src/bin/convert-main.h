// convert-main.h

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
// Converts an fst to another type.  Includes helper function for
// fstconvert.cc that templates the main on the arc type to support
// multiple and extensible arc types.
//

#ifndef FST_CONVERT_MAIN_H__
#define FST_CONVERT_MAIN_H__

#include <string>
#include <fst/main.h>

DECLARE_string(fst_type);

namespace fst {

// Main function for fstconvert templated on the arc type.
// Call only with 'return CALL_FST_MAIN' in main().
template <class Arc>
int ConvertMain(int argc, char **argv, istream &strm,
             const FstReadOptions &opts) {
  Fst<Arc> *ifst = Fst<Arc>::Read(strm, opts);
  if (!ifst) return 1;

  Fst<Arc> *ofst = ifst;
  if (FLAGS_fst_type != ifst->Type()) {
    ofst = Convert<Arc>(*ifst, FLAGS_fst_type);
    if (!ofst) return 1;
  }
  ofst->Write(argc > 2 ? argv[2] : "");

  return 0;
}

}  // namespace fst

#endif  // FST_CONVERT_MAIN_H__
