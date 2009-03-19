// log.h
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
// Google-style logging declarations and inline definitions.

#ifndef FST_LIB_LOG_H__
#define FST_LIB_LOG_H__

#include <cassert>
#include <iostream>
#include <string>

#include <fst/flags.h>

using std::string;

DECLARE_int32(v);

class LogMessage {
 public:
  LogMessage(const string &type) : fatal_(type == "FATAL") {
    std::cerr << type << ": ";
  }
  ~LogMessage() {
    std::cerr << std::endl;
    if(fatal_)
      exit(1);
  }
  std::ostream &stream() { return std::cerr; }

 private:
  bool fatal_;
};

#define LOG(type) LogMessage(#type).stream()
#define VLOG(level) if ((level) <= FLAGS_v) LOG(INFO)

// Checks
#define CHECK(x) assert(x)
#define CHECK_EQ(x, y) assert((x) == (y))


#endif  // FST_LIB_LOG_H__
