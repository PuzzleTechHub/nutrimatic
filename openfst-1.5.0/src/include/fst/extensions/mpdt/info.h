// info.h

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
// Prints information about an MPDT.

#ifndef FST_EXTENSIONS_MPDT_INFO_H__
#define FST_EXTENSIONS_MPDT_INFO_H__

#include <unordered_map>
using std::unordered_map;
using std::unordered_multimap;
#include <unordered_set>
using std::unordered_set;
using std::unordered_multiset;
#include <vector>
using std::vector;

#include <fst/fst.h>
#include <fst/extensions/mpdt/mpdt.h>

namespace fst {

// Compute various information about MPDTs, helper class for mpdtinfo.cc.
template <class A, int nlevels = 2> class MPdtInfo {
public:
  typedef A Arc;
  typedef typename A::StateId StateId;
  typedef typename A::Label Label;
  typedef typename A::Weight Weight;

  MPdtInfo(const Fst<A> &fst,
           const vector<pair<typename A::Label,
                             typename A::Label> > &parens,
           const vector<typename A::Label> &assignments);

  const string& FstType() const { return fst_type_; }
  const string& ArcType() const { return A::Type(); }

  int64 NumStates() const { return nstates_; }
  int64 NumArcs() const { return narcs_; }
  int64 NumLevels() const { return nlevels; }
  int64 NumOpenParens(int lev) const { return nopen_parens_[lev]; }
  int64 NumCloseParens(int lev) const { return nclose_parens_[lev]; }
  int64 NumUniqueOpenParens(int lev) const { return nuniq_open_parens_[lev]; }
  int64 NumUniqueCloseParens(int lev) const { return nuniq_close_parens_[lev]; }
  int64 NumOpenParenStates(int lev) const { return nopen_paren_states_[lev]; }
  int64 NumCloseParenStates(int lev) const { return nclose_paren_states_[lev]; }

  void Print();

 private:
  string fst_type_;
  int64 nstates_;
  int64 narcs_;
  int64 nopen_parens_[nlevels];
  int64 nclose_parens_[nlevels];
  int64 nuniq_open_parens_[nlevels];
  int64 nuniq_close_parens_[nlevels];
  int64 nopen_paren_states_[nlevels];
  int64 nclose_paren_states_[nlevels];
  bool error_;

  DISALLOW_COPY_AND_ASSIGN(MPdtInfo);
};

template <class A, int nlevels>
MPdtInfo<A, nlevels>::MPdtInfo(const Fst<A> &fst,
                               const vector<pair<typename A::Label,
                                                 typename A::Label> > &parens,
                               const vector<typename A::Label> &assignments)
    : fst_type_(fst.Type()),
      nstates_(0),
      narcs_(0),
      error_(false) {
  unordered_map<Label, size_t> paren_map;
  unordered_set<Label> paren_set;
  unordered_map<Label, int> paren_levels;
  unordered_set<StateId> open_paren_state_set;
  unordered_set<StateId> close_paren_state_set;

  if (parens.size() != assignments.size()) {
    FSTERROR() << "MPdtInfo: parens of different size from assignments";
    error_ = true;
    return;
  }
  for (int i = 0; i < assignments.size(); ++i) {
    // Assignments here start at 0, so assuming the human-readable version has
    // them starting at 1, we should subtract 1 here
    int lev = assignments[i] - 1;
    if (lev < 0 || lev >= nlevels) {
      FSTERROR() << "MPdtInfo: specified level " << lev << " out of bounds";
      error_ = true;
      return;
    }
    const pair<Label, Label> &p = parens[i];
    paren_levels[p.first] = lev;
    paren_levels[p.second] = lev;
    paren_map[p.first] = i;
    paren_map[p.second] = i;
  }

  for (int i = 0; i < nlevels; ++i) {
    nopen_parens_[i] = 0;
    nclose_parens_[i] = 0;
    nuniq_open_parens_[i] = 0;
    nuniq_close_parens_[i] = 0;
    nopen_paren_states_[i] = 0;
    nclose_paren_states_[i] = 0;
  }

  for (StateIterator< Fst<A> > siter(fst);
       !siter.Done();
       siter.Next()) {
    ++nstates_;
    StateId s = siter.Value();
    for (ArcIterator< Fst<A> > aiter(fst, s);
         !aiter.Done();
         aiter.Next()) {
      const A &arc = aiter.Value();
      ++narcs_;
      typename unordered_map<Label, size_t>::const_iterator pit
        = paren_map.find(arc.ilabel);
      if (pit != paren_map.end()) {
        Label open_paren =  parens[pit->second].first;
        Label close_paren =  parens[pit->second].second;
        int lev = paren_levels[arc.ilabel];
        if (arc.ilabel == open_paren) {
          ++nopen_parens_[lev];
          if (!paren_set.count(open_paren)) {
            ++nuniq_open_parens_[lev];
            paren_set.insert(open_paren);
          }
          if (!open_paren_state_set.count(arc.nextstate)) {
            ++nopen_paren_states_[lev];
            open_paren_state_set.insert(arc.nextstate);
          }
        } else {
          ++nclose_parens_[lev];
          if (!paren_set.count(close_paren)) {
            ++nuniq_close_parens_[lev];
            paren_set.insert(close_paren);
          }
          if (!close_paren_state_set.count(s)) {
            ++nclose_paren_states_[lev];
            close_paren_state_set.insert(s);
          }
        }
      }
    }
  }
}


template <class A, int nlevels>
void MPdtInfo<A, nlevels>::Print() {
  ios_base::fmtflags old = std::cout.setf(ios::left);
  std::cout.width(50);
  std::cout << "fst type" << FstType().c_str() << std::endl;
  std::cout.width(50);
  std::cout << "arc type" << ArcType().c_str() << std::endl;
  std::cout.width(50);
  std::cout << "# of states" << NumStates() << std::endl;
  std::cout.width(50);
  std::cout << "# of arcs" << NumArcs() << std::endl;
  std::cout.width(50);
  std::cout << "# of levels" << NumLevels() << std::endl;
  std::cout.width(50);
  for (int i = 0; i < nlevels; ++i) {
    int lev = i + 1;
    std::cout << "# of open parentheses at level " << lev << "\t"
              << NumOpenParens(i) << std::endl;
    std::cout.width(50);
    std::cout << "# of close parentheses at level " << lev << "\t"
              << NumCloseParens(i) << std::endl;
    std::cout.width(50);
    std::cout << "# of unique open parentheses at level " << lev << "\t"
              << NumUniqueOpenParens(i) << std::endl;
    std::cout.width(50);
    std::cout << "# of unique close parentheses at level " << lev << "\t"
              << NumUniqueCloseParens(i) << std::endl;
    std::cout.width(50);
    std::cout << "# of open parenthesis dest. states at level " << lev << "\t"
              << NumOpenParenStates(i) << std::endl;
    std::cout.width(50);
    std::cout << "# of close parenthesis source states at level " << lev << "\t"
              << NumCloseParenStates(i) << std::endl;
    std::cout.width(50);
  }
  std::cout.setf(old);
}

}  // namespace fst

#endif  // FST_EXTENSIONS_MPDT_INFO_H__
