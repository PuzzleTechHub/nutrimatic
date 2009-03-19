// text-io.h

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
// Utilities for reading and writing textual strings representing
// states, labels, and weights and files specifying label-label pairs
// and potentials (state-weight pairs).
//

#ifndef FST_TEXT_IO_MAIN_H__
#define FST_TEXT_IO_MAIN_H__

#include <cstring>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <iostream>
#include <fstream>
#include <fst/util.h>

namespace fst {

template <typename Label>
void ReadLabelPairs(const string& filename,
                    vector<pair<Label, Label> >* pairs,
                    bool allow_negative = false) {
  ifstream strm(filename.c_str());
  if (!strm)
    LOG(FATAL) << "ReadLabelPairs: Can't open file: " << filename;

  const int kLineLen = 8096;
  char line[kLineLen];
  size_t nline = 0;

  pairs->clear();
  while (strm.getline(line, kLineLen)) {
    ++nline;
    vector<char *> col;
    SplitToVector(line, "\n\t ", &col, true);
    if (col.size() == 0 || col[0][0] == '\0')  // empty line
      continue;
    if (col.size() != 2)
      LOG(FATAL) << "ReadLabelPairs: Bad number of columns, "
                 << "file = " << filename << ", line = " << nline;

    Label fromlabel = StrToInt64(col[0], filename, nline, allow_negative);
    Label tolabel   = StrToInt64(col[1], filename, nline, allow_negative);
    pairs->push_back(make_pair(fromlabel, tolabel));
  }
}

template <typename Weight>
void ReadPotentials(const string& filename,
                    vector<Weight>* potential) {
  ifstream strm(filename.c_str());
  if (!strm)
    LOG(FATAL) << "ReadPotentials: Can't open file: " << filename;

  const int kLineLen = 8096;
  char line[kLineLen];
  size_t nline = 0;

  potential->clear();
  while (strm.getline(line, kLineLen)) {
    ++nline;
    vector<char *> col;
    SplitToVector(line, "\n\t ", &col, true);
    if (col.size() == 0 || col[0][0] == '\0')  // empty line
      continue;
    if (col.size() != 2)
      LOG(FATAL) << "ReadPotentials: Bad number of columns, "
                 << "file = " << filename << ", line = " << nline;

    ssize_t s = StrToInt64(col[0], filename, nline, false);
    Weight weight = StrToWeight<Weight>(col[1], filename, nline);
    while (potential->size() <= s)
      potential->push_back(Weight::Zero());
    (*potential)[s] = weight;
  }
}

template <typename Weight>
void WritePotentials(const string& filename,
                     const vector<Weight>& potential) {
  LOG(INFO) << "(0)";
  ostream *strm = &std::cout;
  if (!filename.empty()) {
    strm = new ofstream(filename.c_str());
    if (!*strm)
      LOG(FATAL) << "WritePotentials: Can't open file: " << filename;
  }
  LOG(INFO) << "(1)";

  strm->precision(9);
  for (ssize_t s = 0; s < potential.size(); ++s)
    *strm << s << "\t" << potential[s] << "\n";

  if (!*strm)
    LOG(FATAL) << "WritePotentials: Write failed: "
               << (filename.empty() ? "standard output" : filename);
  if (strm != &std::cout)
    delete strm;
}

}  // namespace fst

#endif  // FST_TEXT_IO_MAIN_H__
