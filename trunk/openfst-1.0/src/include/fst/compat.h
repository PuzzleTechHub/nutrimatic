// compat.h
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
// Google compatibility declarations and inline definitions.

#ifndef FST_LIB_COMPAT_H__
#define FST_LIB_COMPAT_H__

#include <dlfcn.h>

#include <climits>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include <config.h>

// Makes copy constructor and operator= private
#define DISALLOW_COPY_AND_ASSIGN(type)    \
  type(const type&);                      \
  void operator=(const type&)

#include <fst/lock.h>
#include <fst/flags.h>
#include <fst/log.h>

using std::cin;
using std::cout;
using std::cerr;
using std::endl;
using std::string;

void FailedNewHandler();

namespace fst {

using namespace std;

void SplitToVector(char *line, const char *delim,
                   std::vector<char *> *vec, bool omit_empty_strings);

// Downcasting
template<typename To, typename From>
inline To down_cast(From* f) {
  return static_cast<To>(f);
}

// Bitcasting
template <class Dest, class Source>
inline Dest bit_cast(const Source& source) {
  // Compile time assertion: sizeof(Dest) == sizeof(Source)
  // A compile error here means your Dest and Source have different sizes.
  typedef char VerifySizesAreEqual [sizeof(Dest) == sizeof(Source) ? 1 :
                                    -1];
  Dest dest;
  memcpy(&dest, &source, sizeof(dest));
  return dest;
}

// Check sums
class CheckSummer {
 public:
  CheckSummer() : count_(0) {
    check_sum_.resize(kCheckSumLength, '\0');
  }

  void Reset() {
    count_ = 0;
    for (int i = 0; i < kCheckSumLength; ++i)
      check_sum_[0] = '\0';
  }

  void Update(void const *data, int size) {
    const char *p = reinterpret_cast<const char *>(data);
    //    const char *p = reinterpret_cast<const char *>(data);
    for (int i = 0; i < size; ++i)
      check_sum_[(count_++) % kCheckSumLength] ^= p[i];
  }

  string Digest() {
    return check_sum_;
  }

 private:
  static const int kCheckSumLength = 32;
  int count_;
  string check_sum_;

  DISALLOW_COPY_AND_ASSIGN(CheckSummer);
};

}  // namespace fst


// Define missing hash functions if needed
#ifndef HAVE_STD__TR1__HASH_LONG_LONG_UNSIGNED_
namespace std {
namespace tr1 {

template <class T> class hash;

template<> struct hash<uint64> {
  size_t operator()(uint64 x) const { return x; }
};

}
}
#endif  // HAVE_STD__TR1__HASH_LONG_LONG_UNSIGNED_

#endif  // FST_LIB_COMPAT_H__
