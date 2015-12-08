// reverse.h

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
// Author: riley@google.com (Michael Riley)
// Author: rws@google.com (Richard Sproat)
//
// \file
// Reverse an MPDT

#ifndef FST_EXTENSIONS_MPDT_REVERSE_H__
#define FST_EXTENSIONS_MPDT_REVERSE_H__

#include <unordered_map>
using std::unordered_map;
using std::unordered_multimap;
#include <vector>
using std::vector;

#include <fst/mutable-fst.h>
#include <fst/relabel.h>
#include <fst/reverse.h>

namespace fst {

// Reverses a multi-stack pushdown transducer (MPDT) encoded as an FST. Also
// reverses the stack assignments
template<class Arc, class RevArc>
void Reverse(const Fst<Arc> &ifst,
             const vector<pair<typename Arc::Label,
             typename Arc::Label> > &parens,
             vector<typename Arc::Label> *assignments,
             MutableFst<RevArc> *ofst) {
  typedef typename Arc::Label Label;

  // Reverses FST
  Reverse(ifst, ofst);

  // Exchanges open and close parenthesis pairs
  vector<pair<Label, Label> > relabel_pairs;
  for (size_t i = 0; i < parens.size(); ++i) {
    relabel_pairs.push_back(std::make_pair(parens[i].first, parens[i].second));
    relabel_pairs.push_back(std::make_pair(parens[i].second, parens[i].first));
  }
  Relabel(ofst, relabel_pairs, relabel_pairs);
  int max_level = -1;
  int min_level = INT_MAX;
  for (int i = 0; i < assignments->size(); ++i) {
    if ((*assignments)[i] < min_level) min_level =  (*assignments)[i];
    if ((*assignments)[i] > max_level) max_level =  (*assignments)[i];
  }
  for (int i = 0; i < assignments->size(); ++i) {
    (*assignments)[i] = (max_level - (*assignments)[i]) + min_level;
  }
}

}  // namespace fst

#endif  // FST_EXTENSIONS_MPDT_REVERSE_H__
