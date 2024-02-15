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
// Projects a transduction onto its input or output language.

#include <cstring>
#include <memory>
#include <string>

#include <fst/flags.h>
#include <fst/log.h>
#include <fst/project.h>
#include <fst/script/fst-class.h>
#include <fst/script/getters.h>
#include <fst/script/project.h>

DECLARE_string(project_type);

int fstproject_main(int argc, char **argv) {
  namespace s = fst::script;
  using fst::ProjectType;
  using fst::script::MutableFstClass;

  std::string usage =
      "Projects a transduction onto its input"
      " or output language.\n\n  Usage: ";
  usage += argv[0];
  usage += " [in.fst [out.fst]]\n";

  SET_FLAGS(usage.c_str(), &argc, &argv, true);
  if (argc > 3) {
    ShowUsage();
    return 1;
  }

  const std::string in_name =
      (argc > 1 && strcmp(argv[1], "-") != 0) ? argv[1] : "";
  const std::string out_name =
      (argc > 2 && strcmp(argv[2], "-") != 0) ? argv[2] : "";

  std::unique_ptr<MutableFstClass> fst(MutableFstClass::Read(in_name, true));
  if (!fst) return 1;

  ProjectType project_type;
  if (!s::GetProjectType(FST_FLAGS_project_type, &project_type)) {
    LOG(ERROR) << argv[0] << ": Unknown or unsupported project type: "
               << FST_FLAGS_project_type;
    return 1;
  }

  s::Project(fst.get(), project_type);

  return !fst->Write(out_name);
}
