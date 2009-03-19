// replace-main.h

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
//  Stand-alone binary for ReplaceFst().
//

#ifndef FST_REPLACE_MAIN_H__
#define FST_REPLACE_MAIN_H__

#include <utility>
#include <vector>
#include <fst/replace.h>
#include <fst/main.h>
#include <fst/vector-fst.h>

DECLARE_bool(epsilon_on_replace);

namespace fst {

// Main function for fstreplace templated on the arc type.
// Call only with 'return CALL_FST_MAIN' in main().
template <class Arc>
int ReplaceMain(int argc, char **argv, istream &strm,
                const FstReadOptions &opts) {
  Fst<Arc> *ifst = Fst<Arc>::Read(strm, opts);
  if (!ifst) return 1;

  typedef typename Arc::Label Label;
  typedef pair<Label, const Fst<Arc>* > FstTuple;
  vector<FstTuple> fst_tuples;
  Label root = atoi(argv[2]);
  fst_tuples.push_back(make_pair(root, ifst));

  for (size_t i = 3; i < argc - 1; i += 2) {
    ifst = Fst<Arc>::Read(argv[i]);
    if (!ifst) return 1;
    Label lab = atoi(argv[i + 1]);
    fst_tuples.push_back(make_pair(lab, ifst));
  }

  VectorFst<Arc> ofst;
  Replace(fst_tuples, &ofst, root, FLAGS_epsilon_on_replace);

  return ofst.Write(argc % 2 == 0 ? argv[argc - 1] : "");
}

}  // namespace fst

#endif  // FST_REPLACE_MAIN_H__
