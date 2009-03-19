// fst-decl.h

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
// This file contains declarations of classes in the Fst template library.
//

#ifndef FST_LIB_FST_DECL_H__
#define FST_LIB_FST_DECL_H__

namespace fst {

class SymbolTable;
class SymbolTableIterator;

template <class W> class FloatWeightTpl;
template <class W> class TropicalWeightTpl;
template <class W> class LogWeightTpl;
template <class W> class MinMaxWeightTpl;

typedef FloatWeightTpl<float> FloatWeight;
typedef TropicalWeightTpl<float> TropicalWeight;
typedef LogWeightTpl<float> LogWeight;
typedef MinMaxWeightTpl<float> MinMaxWeight;

template <class W> class ArcTpl;
typedef ArcTpl<TropicalWeight> StdArc;
typedef ArcTpl<LogWeight> LogArc;

template <class A, class C, class U = uint32> class CompactFst;
template <class A, class U = uint32> class ConstFst;
template <class A> class ExpandedFst;
template <class A> class Fst;
template <class A> class MutableFst;
template <class A> class VectorFst;

template <class A, class C> class ArcSortFst;
template <class A> class ClosureFst;
template <class A> class ComposeFst;
template <class A> class ConcatFst;
template <class A> class DeterminizeFst;
template <class A> class DeterminizeFst;
template <class A> class DifferenceFst;
template <class A> class IntersectFst;
template <class A> class InvertFst;
template <class A, class B, class C> class MapFst;
template <class A> class ProjectFst;
template <class A> class RelabelFst;
template <class A, class T> class ReplaceFst;
template <class A> class RmEpsilonFst;
template <class A> class UnionFst;

template <class T, class Compare> class Heap;

template <class A> class AcceptorCompactor;
template <class A> class StringCompactor;
template <class A> class UnweightedAcceptorCompactor;
template <class A> class UnweightedCompactor;
template <class A> class WeightedStringCompactor;

template <class A, class P> class DefaultReplaceStateTable;

typedef CompactFst<StdArc, AcceptorCompactor<StdArc> >
StdCompactAcceptorFst;
typedef CompactFst< StdArc, StringCompactor<StdArc> >
StdCompactStringFst;
typedef CompactFst<StdArc, UnweightedAcceptorCompactor<StdArc> >
StdCompactUnweightedAcceptorFst;
typedef CompactFst<StdArc, UnweightedCompactor<StdArc> >
StdCompactUnweightedFst;
typedef CompactFst< StdArc, WeightedStringCompactor<StdArc> >
StdCompactWeightedStringFst;
typedef ConstFst<StdArc> StdConstFst;
typedef ExpandedFst<StdArc> StdExpandedFst;
typedef Fst<StdArc> StdFst;
typedef MutableFst<StdArc> StdMutableFst;
typedef VectorFst<StdArc> StdVectorFst;

template <class C> class StdArcSortFst;
typedef ClosureFst<StdArc> StdClosureFst;
typedef ComposeFst<StdArc> StdComposeFst;
typedef ConcatFst<StdArc> StdConcatFst;
typedef DeterminizeFst<StdArc> StdDeterminizeFst;
typedef DifferenceFst<StdArc> StdDifferenceFst;
typedef IntersectFst<StdArc> StdIntersectFst;
typedef InvertFst<StdArc> StdInvertFst;
typedef ProjectFst<StdArc> StdProjectFst;
typedef RelabelFst<StdArc> StdRelabelFst;
typedef ReplaceFst<StdArc, DefaultReplaceStateTable<StdArc, ssize_t> >
StdReplaceFst;
typedef RmEpsilonFst<StdArc> StdRmEpsilonFst;
typedef UnionFst<StdArc> StdUnionFst;

}

#endif  // FST_LIB_FST_DECL_H__
