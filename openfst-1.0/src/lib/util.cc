// util.cc

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
// FST utility definitions.

#include <string>
#include <fst/util.h>

namespace fst {

int64 StrToInt64(const string &s, const string &src, size_t nline,
                 bool allow_negative = false) {
  int64 n;
  const char *cs = s.c_str();
  char *p;
  n = strtoll(cs, &p, 10);
  if (p < cs + s.size() || (!allow_negative && n < 0))
    LOG(FATAL) << "StrToInt64: Bad integer = " << s
               << "\", source = " << src << ", line = " << nline;
  return n;
}

void Int64ToStr(int64 n, string *s) {
  const int kNumLen = 128;
  char nstr[kNumLen];
  snprintf(nstr, kNumLen, "%lld", n);
  *s += nstr;
}

}  // namespace fst
