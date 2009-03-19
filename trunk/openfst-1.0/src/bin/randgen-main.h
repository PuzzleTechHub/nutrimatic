// randgen-main.h

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
// Generates random paths through an FST. Includes helper function for
// fstrandgen.cc that templates the main on the arc type to support
// multiple and extensible arc types.

// Generates random paths through an FST.


#ifndef FST_RANDGEN_MAIN_H__
#define FST_RANDGEN_MAIN_H__

#include <fst/main.h>
#include <fst/randgen.h>
#include <fst/vector-fst.h>

DECLARE_int32(max_length);
DECLARE_int32(npath);
DECLARE_int32(seed);
DECLARE_string(select);

namespace fst {

// This version compiles with any arc type, but issues run-time
// error for inappropriate weight types.
template <class A>
struct MainLogProbArcSelector {
  typedef typename A::StateId StateId;
  typedef typename A::Weight Weight;

  MainLogProbArcSelector(int seed = time(0)) {
    LOG(FATAL) << "MainLogProbArcSelector: bad weight type: "
               << Weight::Type();
  }
  size_t operator()(const Fst<A> &fst, StateId s) const { return 0; }
};

template <>
struct MainLogProbArcSelector<StdArc> : public LogProbArcSelector<StdArc> {
  MainLogProbArcSelector(int seed = time(0))
      : LogProbArcSelector<StdArc>(seed) {}
};

template <>
struct MainLogProbArcSelector<LogArc> : public LogProbArcSelector<LogArc> {
  MainLogProbArcSelector(int seed = time(0))
      : LogProbArcSelector<LogArc>(seed) {}
};


// Main function for fstrandgen templated on the arc type.
// Call only with 'return CALL_FST_MAIN' in main().
template <class Arc>
int RandGenMain(int argc, char **argv, istream &istrm,
                const FstReadOptions &iopts) {
  Fst<Arc> *ifst = Fst<Arc>::Read(istrm, iopts);
  if (!ifst) return 1;

  VectorFst<Arc> ofst;

  if (FLAGS_select == "uniform") {
    RandGenOptions< UniformArcSelector<Arc> >
        ropts(UniformArcSelector<Arc>(FLAGS_seed), FLAGS_max_length,
              FLAGS_npath);
    RandGen(*ifst, &ofst, ropts);
  } else if (FLAGS_select == "log_prob") {
    RandGenOptions< MainLogProbArcSelector<Arc> >
        ropts(MainLogProbArcSelector<Arc>(FLAGS_seed), FLAGS_max_length,
              FLAGS_npath);
    RandGen(*ifst, &ofst, ropts);
  } else {
    LOG(ERROR) << argv[0] << ": Unknown selection type \""
               << FLAGS_select << "\"\n";
    return 1;
  }

  ofst.Write(argc > 2 ? argv[2] : "");
  return 0;
}

}  // namespace fst

#endif  // FST_RANDGEN_MAIN_H__
