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
// Prints out various information about an FST such as number of states
// and arcs and property values (see properties.h).

#include <cstring>
#include <ios>
#include <iostream>
#include <memory>
#include <string>

#include <fst/flags.h>
#include <fst/log.h>
#include <fstream>
#include <fst/fst.h>
#include <fst/script/arcfilter-impl.h>
#include <fst/script/fst-class.h>
#include <fst/script/getters.h>
#include <fst/script/info-impl.h>
#include <fst/script/info.h>

DECLARE_string(arc_filter);
DECLARE_string(info_type);
DECLARE_bool(test_properties);
DECLARE_bool(fst_verify);

namespace {
// Prints info using only the header of the FST with path `in_name`.
// Returns true on success.
bool PrintHeaderInfo(const std::string &in_name) {
  fst::FstHeader header;
  if (in_name.empty()) {
    if (!header.Read(std::cin, "stdin")) {
      LOG(ERROR) << "fstinfo: Unable to read header from stdin";
      return false;
    }
  } else {
    std::ifstream istrm;
    istrm.open(in_name, std::ios_base::in | std::ios_base::binary);
    if (!istrm) {
      LOG(ERROR) << "fstinfo: Unable to open " << in_name;
      return false;
    }
    if (!header.Read(istrm, in_name)) {
      LOG(ERROR) << "fstinfo: Unable to read header from " << in_name;
      return false;
    }
    if (!istrm) {
      LOG(ERROR) << "fstinfo: Unable to close " << in_name;
      return false;
    }
  }

  fst::PrintHeader(std::cout, header);
  return true;
}
}  // namespace

int fstinfo_main(int argc, char **argv) {
  namespace s = fst::script;
  using fst::script::FstClass;

  std::string usage = "Prints out information about an FST.\n\n  Usage: ";
  usage += argv[0];
  usage += " [in.fst]\n";

  SET_FLAGS(usage.c_str(), &argc, &argv, true);
  if (argc > 2) {
    ShowUsage();
    return 1;
  }

  const std::string in_name =
      (argc > 1 && (strcmp(argv[1], "-") != 0)) ? argv[1] : "";

  if (FST_FLAGS_info_type == "fast") {
    if (!PrintHeaderInfo(in_name)) return 1;
  } else {
    std::unique_ptr<FstClass> ifst(FstClass::Read(in_name));
    if (!ifst) return 1;

    s::ArcFilterType arc_filter;
    if (!s::GetArcFilterType(FST_FLAGS_arc_filter, &arc_filter)) {
      LOG(ERROR) << argv[0] << ": Unknown or unsupported arc filter type "
                 << FST_FLAGS_arc_filter;
      return 1;
    }
    s::Info(*ifst, FST_FLAGS_test_properties, arc_filter,
            FST_FLAGS_info_type, FST_FLAGS_fst_verify);
  }

  return 0;
}
