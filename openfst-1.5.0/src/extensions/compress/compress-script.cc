
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

// Definitions of 'scriptable' versions of compression operations, that is,
// those that can be called with FstClass-type arguments.

// See comments in nlp/fst/script/script-impl.h for how the registration
// mechanism allows these to work with various arc types.

#include <vector>
using std::vector;
#include <utility>
using std::pair; using std::make_pair;

#include <fst/extensions/compress/compress-script.h>

#include <fst/script/script-impl.h>

namespace fst {
namespace script {

void Compress(const FstClass &fst,
              const string &filename,
              const bool gzip) {
  CompressArgs args(fst, filename, gzip);
  Apply<Operation<CompressArgs> >("Compress", fst.ArcType(), &args);
}

void Decompress(const string &filename,
                MutableFstClass *fst,
                const bool gzip) {
  DecompressArgs args(filename, fst, gzip);
  Apply<Operation<DecompressArgs> >("Decompress", fst->ArcType(), &args);
}

// Register operations for common arc types.

REGISTER_FST_OPERATION(Compress, StdArc, CompressArgs);
REGISTER_FST_OPERATION(Compress, LogArc, CompressArgs);
REGISTER_FST_OPERATION(Compress, Log64Arc, CompressArgs);

REGISTER_FST_OPERATION(Decompress, StdArc, DecompressArgs);
REGISTER_FST_OPERATION(Decompress, LogArc, DecompressArgs);
REGISTER_FST_OPERATION(Decompress, Log64Arc, DecompressArgs);


}  // namespace script
}  // namespace fst
