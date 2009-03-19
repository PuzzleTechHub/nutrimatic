// fstlib.h

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
// \page FstLib FST - Weighted Finite State Transducers
// This is a library for constructing, combining, optimizing, and
// searching "weighted finite-state transducers" (FSTs). Weighted
// finite-state transducers are automata where each transition has an
// input label, an output label, and a weight. The more familiar
// finite-state acceptor is represented as a transducer with each
// transition's input and output the same.  Finite-state acceptors
// are used to represent sets of strings (specifically, "regular" or
// "rational sets"); finite-state transducers are used to represent
// binary relations between pairs of strings (specifically, "rational
// transductions"). The weights can be used to represent the cost of
// taking a particular transition.
//
// In this library, the transducers are templated on the Arc
// (transition) definition, which allows changing the label, weight,
// and state ID sets. Labels and state IDs are restricted to signed
// integral types but the weight can be an arbitrary type whose
// members satisfy certain algebraic ("semiring") properties.
//
// For more information, see the FST Library Wiki page:
// http://wiki.corp.google.com/twiki/bin/view/Main/FstLibrary

// \file
// This convenience file includes all other FST inl.h files.
//

#ifndef FST_LIB_FSTLIB_H__
#define FST_LIB_FSTLIB_H__

// Abstract FST classes
#include <fst/fst.h>
#include <fst/expanded-fst.h>
#include <fst/mutable-fst.h>

// Concrete FST classes
#include <fst/vector-fst.h>
#include <fst/const-fst.h>

// FST algorithms and delayed FST classes
#include <fst/arcsort.h>
#include <fst/closure.h>
#include <fst/compose.h>
#include <fst/concat.h>
#include <fst/connect.h>
#include <fst/determinize.h>
#include <fst/difference.h>
#include <fst/encode.h>
#include <fst/epsnormalize.h>
#include <fst/equal.h>
#include <fst/equivalent.h>
#include <fst/factor-weight.h>
#include <fst/intersect.h>
#include <fst/invert.h>
#include <fst/map.h>
#include <fst/minimize.h>
#include <fst/project.h>
#include <fst/prune.h>
#include <fst/push.h>
#include <fst/randequivalent.h>
#include <fst/randgen.h>
#include <fst/relabel.h>
#include <fst/replace.h>
#include <fst/reverse.h>
#include <fst/reweight.h>
#include <fst/rmepsilon.h>
#include <fst/rmfinalepsilon.h>
#include <fst/shortest-distance.h>
#include <fst/shortest-path.h>
#include <fst/synchronize.h>
#include <fst/topsort.h>
#include <fst/union.h>
#include <fst/verify.h>
#include <fst/visit.h>

#endif  // FST_LIB_FSTLIB_H__
