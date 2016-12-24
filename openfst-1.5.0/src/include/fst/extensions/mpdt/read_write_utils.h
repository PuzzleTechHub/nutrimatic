// read_write_utils.h

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
// Author: rws@google.com (Richard Sproat)
//
// \file
// Definition of ReadLabelTriples based on ReadLabelPairs, like that in
// nlp/fst/lib/util.h for pairs, and similarly for WriteLabelTriples.

#ifndef FST_EXTENSIONS_MPDT_READ_WRITE_UTILS_H__
#define FST_EXTENSIONS_MPDT_READ_WRITE_UTILS_H__

#include <sstream>
#include <string>
#include <utility>
using std::pair; using std::make_pair;
#include <vector>
using std::vector;

#include <fst/test-properties.h>

#include <iostream>
#include <fstream>
#include <sstream>


namespace fst {

// Returns true on success
template <typename Label>
bool ReadLabelTriples(
    const string& filename,
    vector<pair<Label, Label> >* pairs,
    vector<Label>* assignments,
    bool allow_negative = false) {
  ifstream strm(filename.c_str());

  if (!strm) {
    LOG(ERROR) << "ReadIntTriples: Can't open file: " << filename;
    return false;
  }

  const int kLineLen = 8096;
  char line[kLineLen];
  size_t nline = 0;

  pairs->clear();
  while (strm.getline(line, kLineLen)) {
    ++nline;
    vector<char *> col;
    SplitToVector(line, "\n\t ", &col, true);
    // empty line or comment?
    if (col.size() == 0 || col[0][0] == '\0' || col[0][0] == '#')
      continue;
    if (col.size() != 3) {
      LOG(ERROR) << "ReadLabelTriples: Bad number of columns, "
                 << "file = " << filename << ", line = " << nline;
      return false;
    }

    bool err;
    Label i1 = StrToInt64(col[0], filename, nline, allow_negative, &err);
    if (err) return false;
    Label i2 = StrToInt64(col[1], filename, nline, allow_negative, &err);
    if (err) return false;
    Label i3 = StrToInt64(col[2], filename, nline, allow_negative, &err);
    if (err) return false;
    pairs->push_back(std::make_pair(i1, i2));
    assignments->push_back(i3);
  }
  return true;
}

// Returns true on success
template <typename Label>
bool WriteLabelTriples(const string& filename,
                       const vector<pair<Label, Label> >& pairs,
                       const vector<Label>& assignments) {
  ostream* strm = &std::cout;
  if (pairs.size() != assignments.size()) {
    LOG(ERROR) <<
        "WriteLabelTriples: pairs and assignments of different sizes";
    return false;
  }

  if (!filename.empty()) {
    strm = new ofstream(filename.c_str());
    if (!*strm) {
      LOG(ERROR) << "WriteLabelTriples: Can't open file: " << filename;
      return false;
    }
  }

  for (ssize_t n = 0; n < pairs.size(); ++n)
    *strm << pairs[n].first << "\t"
          << pairs[n].second << "\t"
          << assignments[n] << "\n";

  if (!*strm) {
    LOG(ERROR) << "WriteLabelTriples: Write failed: "
               << (filename.empty() ? "standard output" : filename);
    return false;
  }
  if (strm != &std::cout) delete strm;
  return true;
}

}  // namespace fst

#endif  // FST_EXTENSIONS_MPDT_READ_WRITE_UTILS_H__
