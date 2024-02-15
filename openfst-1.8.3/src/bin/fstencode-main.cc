// Copyright 2005-2024 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the 'License');
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an 'AS IS' BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Encode transducer labels and/or weights.

#include <cstring>
#include <memory>
#include <string>

#include <fst/flags.h>
#include <fst/script/decode.h>
#include <fst/script/encode.h>
#include <fst/script/encodemapper-class.h>
#include <fst/script/fst-class.h>
#include <fst/script/getters.h>

DECLARE_bool(decode);
DECLARE_bool(encode_labels);
DECLARE_bool(encode_weights);
DECLARE_bool(encode_reuse);

int fstencode_main(int argc, char **argv) {
  namespace s = fst::script;
  using fst::script::EncodeMapperClass;
  using fst::script::MutableFstClass;

  std::string usage = "Encodes transducer labels and/or weights.\n\n  Usage: ";
  usage += argv[0];
  usage += " in.fst mapper [out.fst]\n";

  SET_FLAGS(usage.c_str(), &argc, &argv, true);
  if (argc < 3 || argc > 4) {
    ShowUsage();
    return 1;
  }

  const std::string in_name = (strcmp(argv[1], "-") != 0) ? argv[1] : "";
  const std::string mapper_name = argv[2];
  const std::string out_name =
      argc > 3 && strcmp(argv[3], "-") != 0 ? argv[3] : "";

  std::unique_ptr<MutableFstClass> fst(MutableFstClass::Read(in_name, true));
  if (!fst) return 1;

  if (FST_FLAGS_decode) {
    std::unique_ptr<EncodeMapperClass> mapper(
        EncodeMapperClass::Read(mapper_name));
    if (!mapper) return 1;
    s::Decode(fst.get(), *mapper);
  } else if (FST_FLAGS_encode_reuse) {
    std::unique_ptr<EncodeMapperClass> mapper(
        EncodeMapperClass::Read(mapper_name));
    if (!mapper) return 1;
    s::Encode(fst.get(), mapper.get());
  } else {
    const auto flags = s::GetEncodeFlags(FST_FLAGS_encode_labels,
                                         FST_FLAGS_encode_weights);
    EncodeMapperClass mapper(fst->ArcType(), flags);
    s::Encode(fst.get(), &mapper);
    if (!mapper.Write(mapper_name)) return 1;
  }

  return !fst->Write(out_name);
}
