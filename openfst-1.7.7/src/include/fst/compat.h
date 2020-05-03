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
// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.

#ifndef FST_LIB_COMPAT_H_
#define FST_LIB_COMPAT_H_

#include <climits>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#if defined(__GNUC__) || defined(__clang__)
#define OPENFST_DEPRECATED(message) __attribute__((deprecated(message)))
#elif defined(_MSC_VER)
#define OPENFST_DEPRECATED(message) __declspec(deprecated(message))
#else
#define OPENFST_DEPRECATED(message)
#endif

#include <fst/config.h>
#include <fst/types.h>
#include <fst/lock.h>
#include <fst/flags.h>
#include <fst/log.h>
#include <fst/icu.h>

void FailedNewHandler();

namespace fst {

// Downcasting.
template <typename To, typename From>
inline To down_cast(From *f) {
  return static_cast<To>(f);
}

// Bitcasting.
template <class Dest, class Source>
inline Dest bit_cast(const Source &source) {
  static_assert(sizeof(Dest) == sizeof(Source),
                "Bitcasting unsafe for specified types");
  Dest dest;
  memcpy(&dest, &source, sizeof(dest));
  return dest;
}

// Checksums.
class CheckSummer {
 public:
  CheckSummer();

  void Reset();

  void Update(void const *data, int size);

  void Update(std::string const &data);

  std::string Digest() { return check_sum_; }

 private:
  constexpr static int kCheckSumLength = 32;
  int count_;
  std::string check_sum_;

  CheckSummer(const CheckSummer &) = delete;
  CheckSummer &operator=(const CheckSummer &) = delete;
};

// Defines make_unique and make_unique_default_init using a standard definition
// that should be compatible with the C++14 and C++20 (respectively)
// definitions.
// TODO(kbg): Remove these once we migrate to C++14 and C++20.

template <typename T, typename... Args>
std::unique_ptr<T> make_unique(Args &&... args) {
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

template <typename T>
std::unique_ptr<T[]> make_unique(size_t n) {
  return std::unique_ptr<T>(new typename std::remove_extent<T>::type[n]());
}

template <typename T>
std::unique_ptr<T> make_unique_default_init() {
  return std::unique_ptr<T>(new T);
}

template <typename T>
std::unique_ptr<T[]> make_unique_default_init(size_t n) {
  return std::unique_ptr<T>(new typename std::remove_extent<T>::type[n]);
}

template <typename T>
std::unique_ptr<T> WrapUnique(T *ptr) {
  return std::unique_ptr<T>(ptr);
}

// String munging.

std::string StringJoin(const std::vector<std::string> &elements,
                       const std::string &delim);

std::string StringJoin(const std::vector<std::string> &elements,
                       const char *delim);

std::string StringJoin(const std::vector<std::string> &elements, char delim);

std::vector<std::string> StringSplit(const std::string &full,
                                     const std::string &delim);

std::vector<std::string> StringSplit(const std::string &full,
                                     const char *delim);

std::vector<std::string> StringSplit(const std::string &full, char delim);

void StripTrailingAsciiWhitespace(std::string *full);

std::string StripTrailingAsciiWhitespace(const std::string &full);

}  // namespace fst

#endif  // FST_LIB_COMPAT_H_
