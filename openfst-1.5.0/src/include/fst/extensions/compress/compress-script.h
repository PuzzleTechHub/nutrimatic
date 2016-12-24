
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

// Declarations of 'scriptable' versions of compression operations, that is,
// those that can be called with FstClass-type arguments.

#ifndef FST_EXTENSIONS_COMPRESS_COMPRESS_SCRIPT_H_
#define FST_EXTENSIONS_COMPRESS_COMPRESS_SCRIPT_H_

#include <utility>
using std::pair; using std::make_pair;
#include <vector>
using std::vector;

#include <fst/extensions/compress/compress.h>

#include <fst/util.h>
#include <fst/script/fst-class.h>
#include <fst/script/arg-packs.h>

namespace fst {
namespace script {

typedef args::Package<const FstClass &,
                      const string &,
                      const bool> CompressArgs;

template<class Arc>
void Compress(CompressArgs *args) {
  const Fst<Arc> &fst = *(args->arg1.GetFst<Arc>());
  const string &filename = args->arg2;
  const bool gzip = args->arg3;

  if (!fst::Compress(fst, filename, gzip))
    FSTERROR() << "Compress: failed";
}

void Compress(const FstClass &fst, const string &filename, const bool gzip);


typedef args::Package<const string &,
                      MutableFstClass *,
                      const bool> DecompressArgs;

template<class Arc>
void Decompress(DecompressArgs *args) {
  const string &filename = args->arg1;
  MutableFst<Arc> *fst = args->arg2->GetMutableFst<Arc>();
  const bool gzip = args->arg3;

  if (!fst::Decompress(filename, fst, gzip))
    FSTERROR() << "Decompress: failed";
}

void Decompress(const string &filename, MutableFstClass *fst, const bool gzip);


}  // namespace script
}  // namespace fst

#endif  // FST_EXTENSIONS_COMPRESS_COMPRESS_SCRIPT_H_
