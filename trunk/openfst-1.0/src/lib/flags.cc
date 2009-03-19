// flags.cc
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
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
// Google-style flag handling definitions

#include <cstring>

#include <fst/compat.h>
#include <fst/flags.h>

DEFINE_int32(v, 0, "verbose level");
DEFINE_bool(help, false, "verbose level");
DEFINE_string(tmpdir, "/tmp/", "temporary directory");

using namespace std;

static string flag_usage;

void SetFlags(const char *usage, int *argc, char ***argv, bool remove_flags) {
  flag_usage = usage;
  int index = 1;
  for (; index < *argc; ++index) {
    string argval = (*argv)[index];

    if (argval[0] != '-' || argval == "-")
      break;
    while (argval[0] == '-')
      argval = argval.substr(1);  // remove initial '-'s

    string arg = argval;
    string val = "";

    // split argval (arg=val) into arg and val
    int pos = argval.find("=");
    if (pos != string::npos) {
      arg = argval.substr(0, pos);
      val = argval.substr(pos + 1);
    }

    FlagRegister<bool> *bool_register =
      FlagRegister<bool>::GetRegister();
    if (bool_register->SetFlag(arg, val))
      continue;
    FlagRegister<string> *string_register =
      FlagRegister<string>::GetRegister();
    if (string_register->SetFlag(arg, val))
      continue;
    FlagRegister<int32> *int32_register =
      FlagRegister<int32>::GetRegister();
    if (int32_register->SetFlag(arg, val))
      continue;
    FlagRegister<int64> *int64_register =
      FlagRegister<int64>::GetRegister();
    if (int64_register->SetFlag(arg, val))
      continue;
    FlagRegister<double> *double_register =
      FlagRegister<double>::GetRegister();
    if (double_register->SetFlag(arg, val))
      continue;

    LOG(FATAL) << "SetFlags: Bad option: " << (*argv)[index];
  }

  if (remove_flags) {
    for (int i = 0; i < *argc - index; ++i)
      (*argv)[i + 1] = (*argv)[i + index];
    *argc -= index - 1;
  }

  if (FLAGS_help) {
    ShowUsage();
    exit(1);
  }
}

void ShowUsage() {
  cout << flag_usage << "\n";
  cout << "  Flags Description:\n";
  FlagRegister<bool> *bool_register = FlagRegister<bool>::GetRegister();
  bool_register->ShowUsage();
  FlagRegister<string> *string_register = FlagRegister<string>::GetRegister();
  string_register->ShowUsage();
  FlagRegister<int32> *int32_register = FlagRegister<int32>::GetRegister();
  int32_register->ShowUsage();
  FlagRegister<int64> *int64_register = FlagRegister<int64>::GetRegister();
  int64_register->ShowUsage();
  FlagRegister<double> *double_register = FlagRegister<double>::GetRegister();
  double_register->ShowUsage();
}
