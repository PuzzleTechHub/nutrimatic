
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
// Copyright 2005-2010 Google, Inc.
// Author: riley@google.com (Michael Riley)

#ifndef FST_SCRIPT_ISOMORPHIC_H_
#define FST_SCRIPT_ISOMORPHIC_H_

#include <fst/script/arg-packs.h>
#include <fst/script/fst-class.h>
#include <fst/isomorphic.h>

namespace fst {
namespace script {

typedef args::Package<const FstClass&, const FstClass&, float, bool *>
IsomorphicInnerArgs;
typedef args::WithReturnValue<bool, IsomorphicInnerArgs> IsomorphicArgs;

template<class Arc>
void Isomorphic(IsomorphicArgs *args) {
  const Fst<Arc> &fst1 = *(args->args.arg1.GetFst<Arc>());
  const Fst<Arc> &fst2 = *(args->args.arg2.GetFst<Arc>());

  args->retval = Isomorphic(fst1, fst2, args->args.arg3, args->args.arg4);
}

bool Isomorphic(const FstClass &fst1, const FstClass &fst2,
           float delta = kDelta, bool *error = 0);

}  // namespace script
}  // namespace fst


#endif  // FST_SCRIPT_ISOMORPHIC_H_
