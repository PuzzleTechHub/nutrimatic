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
// Google compatibility definitions.

#include <fst/compat.h>

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <numeric>

void FailedNewHandler() {
  std::cerr << "Memory allocation failed" << std::endl;
  std::exit(1);
}

namespace fst {

CheckSummer::CheckSummer() : count_(0) {
  check_sum_.resize(kCheckSumLength, '\0');
}

void CheckSummer::Reset() {
  count_ = 0;
  for (int i = 0; i < kCheckSumLength; ++i) check_sum_[i] = '\0';
}

void CheckSummer::Update(void const *data, int size) {
  const char *p = reinterpret_cast<const char *>(data);
  for (int i = 0; i < size; ++i) {
    check_sum_[(count_++) % kCheckSumLength] ^= p[i];
  }
}

void CheckSummer::Update(std::string const &data) {
  for (int i = 0; i < data.size(); ++i) {
    check_sum_[(count_++) % kCheckSumLength] ^= data[i];
  }
}

// String joining and splitting.

namespace {

// Computes size of joined string.
size_t GetResultSize(const std::vector<std::string> &elements, size_t s_size) {
  const auto lambda = [](size_t partial, const std::string &right) {
    return partial + right.size();
  };
  return (std::accumulate(elements.begin(), elements.end(), 0, lambda) +
          s_size * (elements.size() - 1));
}

}  // namespace

// Joins a vector of strings on a given delimiter.

std::string StringJoin(const std::vector<std::string> &elements,
                       const std::string &delim) {
  std::string result;
  if (elements.empty()) return result;
  size_t s_size = delim.size();
  result.reserve(GetResultSize(elements, s_size));
  auto it = elements.begin();
  result.append(it->data(), it->size());
  for (++it; it != elements.end(); ++it) {
    result.append(delim.data(), s_size);
    result.append(it->data(), it->size());
  }
  return result;
}

std::string StringJoin(const std::vector<std::string> &elements,
                       const char *delim) {
  const std::string str_delim(delim);
  return StringJoin(elements, str_delim);
}

std::string StringJoin(const std::vector<std::string> &elements, char delim) {
  const std::string str_delim{delim};
  return StringJoin(elements, str_delim);
}

// Splits a string according to delimiter, skipping over consecutive
// delimiters.

std::vector<std::string> StringSplit(const std::string &full,
                                     const std::string &delim) {
  size_t prev = 0;
  size_t found = full.find_first_of(delim);
  size_t size = found - prev;
  std::vector<std::string> result;
  if (size > 0) result.push_back(full.substr(prev, size));
  while (found != std::string::npos) {
    prev = found + 1;
    found = full.find_first_of(delim, prev);
    size = found - prev;
    if (size > 0) result.push_back(full.substr(prev, size));
  }
  return result;
}

std::vector<std::string> StringSplit(const std::string &full,
                                     const char *delim) {
  const std::string str_delim(delim);
  return StringSplit(full, str_delim);
}

std::vector<std::string> StringSplit(const std::string &full, char delim) {
  const std::string str_delim{delim};
  return StringSplit(full, str_delim);
}

void StripTrailingAsciiWhitespace(std::string *full) {
  const auto lambda = [](char ch) { return !std::isspace(ch); };
  const auto pos = std::find_if(full->rbegin(), full->rend(), lambda).base();
  full->erase(pos, full->end());
}

std::string StripTrailingAsciiWhitespace(const std::string &full) {
  std::string copy(full);
  StripTrailingAsciiWhitespace(&copy);
  return copy;
}

}  // namespace fst
