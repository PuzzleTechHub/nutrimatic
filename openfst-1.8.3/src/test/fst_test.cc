// Copyright 2005-2024 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the 'License');
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an 'AS IS' BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Regression test for FST classes.

#include <fst/fst.h>

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <ostream>
#include <string>

#include <fst/log.h>
#include <fst/arc.h>
#include <fst/cache.h>
#include <fst/compact-fst.h>
#include <fst/const-fst.h>
#include <fst/edit-fst.h>
#include <fst/float-weight.h>
#include <fst/fst-decl.h>
#include <fst/matcher-fst.h>
#include <fst/pair-weight.h>
#include <fst/product-weight.h>
#include <fst/register.h>
#include <fst/vector-fst.h>
#include <fst/test/compactors.h>
#include <fst/test/fst_test.h>

namespace fst {
namespace {

// A user-defined arc type.
struct CustomArc {
  using Label = int16_t;
  using Weight = ProductWeight<TropicalWeight, LogWeight>;
  using StateId = int64_t;

  CustomArc(Label i, Label o, Weight w, StateId s)
      : ilabel(i), olabel(o), weight(w), nextstate(s) {}
  CustomArc() = default;

  static const std::string &Type() {  // Arc type name
    static const std::string *const type = new std::string("my");
    return *type;
  }

  Label ilabel;       // Transition input label
  Label olabel;       // Transition output label
  Weight weight;      // Transition weight
  StateId nextstate;  // Transition destination state
};

REGISTER_FST(VectorFst, CustomArc);
REGISTER_FST(ConstFst, CustomArc);
static fst::FstRegisterer<
    CompactArcFst<StdArc, TrivialArcCompactor<StdArc>>>
    CompactFst_StdArc_TrivialCompactor_registerer;
static fst::FstRegisterer<
    CompactArcFst<CustomArc, TrivialArcCompactor<CustomArc>>>
    CompactFst_CustomArc_TrivialCompactor_registerer;
static fst::FstRegisterer<ConstFst<StdArc, uint16_t>>
    ConstFst_StdArc_uint16_registerer;
static fst::FstRegisterer<
    CompactArcFst<StdArc, TrivialArcCompactor<StdArc>, uint16_t>>
    CompactFst_StdArc_TrivialCompactor_uint16_registerer;
static fst::FstRegisterer<CompactFst<StdArc, TrivialCompactor<StdArc>>>
    CompactFst_StdArc_CustomCompactor_registerer;
static fst::FstRegisterer<
    CompactFst<CustomArc, TrivialCompactor<CustomArc>>>
    CompactFst_CustomArc_CustomCompactor_registerer;

}  // namespace
}  // namespace fst

using fst::CompactArcFst;
using fst::CompactFst;
using fst::ConstFst;
using fst::CustomArc;
using fst::EditFst;
using fst::FstTester;
using fst::StdArc;
using fst::StdArcLookAheadFst;
using fst::TrivialArcCompactor;
using fst::TrivialCompactor;
using fst::VectorFst;

int main(int argc, char **argv) {
  SetFlag(&FST_FLAGS_fst_verify_properties, true);
  SET_FLAGS(argv[0], &argc, &argv, true);

  LOG(INFO) << "Testing VectorFst<StdArc>.";
  {
    for (const size_t num_states : {0, 1, 2, 3, 128}) {
      FstTester<VectorFst<StdArc>> std_vector_tester(num_states);
      std_vector_tester.TestBase();
      std_vector_tester.TestExpanded();
      std_vector_tester.TestAssign();
      std_vector_tester.TestCopy();
      std_vector_tester.TestIO();
      std_vector_tester.TestMutable();
    }

    LOG(INFO) << "Testing empty StdVectorFst.";
    FstTester<VectorFst<StdArc>> empty_tester(/*num_states=*/0);
    {
      const VectorFst<StdArc> empty_fst;
      empty_tester.TestBase(empty_fst);
      empty_tester.TestExpanded(empty_fst);
      empty_tester.TestCopy(empty_fst);
      empty_tester.TestIO(empty_fst);
      empty_tester.TestAssign(empty_fst);
    }
    {
      VectorFst<StdArc> empty_fst;
      empty_tester.TestMutable(&empty_fst);
    }
  }

  LOG(INFO) << "Testing ConstFst<StdArc>.";
  {
    FstTester<ConstFst<StdArc>> std_const_tester;
    std_const_tester.TestBase();
    std_const_tester.TestExpanded();
    std_const_tester.TestCopy();
    std_const_tester.TestIO();
  }

  LOG(INFO) << "Testing CompactArcFst<StdArc, TrivialArcCompactor<StdArc>>.";
  {
    FstTester<CompactArcFst<StdArc, TrivialArcCompactor<StdArc>>>
        std_compact_tester;
    std_compact_tester.TestBase();
    std_compact_tester.TestExpanded();
    std_compact_tester.TestCopy();
    std_compact_tester.TestIO();
  }

  LOG(INFO) << "Testing CompactFst<StdArc, TrivialArcCompactor<StdArc>>.";
  {
    for (const size_t num_states : {0, 1, 2, 3, 128}) {
      FstTester<CompactFst<StdArc, TrivialCompactor<StdArc>>>
          std_compact_tester(num_states);
      std_compact_tester.TestBase();
      std_compact_tester.TestExpanded();
      std_compact_tester.TestCopy();
      std_compact_tester.TestIO();
    }

    LOG(INFO) << "Testing empty CompactFst.";
    FstTester<CompactFst<StdArc, TrivialCompactor<StdArc>>> empty_tester(
        /*num_states=*/0);
    {
      const CompactFst<StdArc, TrivialCompactor<StdArc>> empty_fst;
      empty_tester.TestBase(empty_fst);
      empty_tester.TestExpanded(empty_fst);
      empty_tester.TestCopy(empty_fst);
      empty_tester.TestIO(empty_fst);
    }
  }

  // VectorFst<CustomArc> tests
  {
    FstTester<VectorFst<CustomArc>> std_vector_tester;
    std_vector_tester.TestBase();
    std_vector_tester.TestExpanded();
    std_vector_tester.TestAssign();
    std_vector_tester.TestCopy();
    std_vector_tester.TestIO();
    std_vector_tester.TestMutable();
  }

  // ConstFst<CustomArc> tests
  {
    FstTester<ConstFst<CustomArc>> std_const_tester;
    std_const_tester.TestBase();
    std_const_tester.TestExpanded();
    std_const_tester.TestCopy();
    std_const_tester.TestIO();
  }

  // CompactArcFst<CustomArc, TrivialArcCompactor<CustomArc>>
  {
    for (const size_t num_states : {0, 1, 2, 3, 128}) {
      FstTester<CompactArcFst<CustomArc, TrivialArcCompactor<CustomArc>>>
          std_compact_tester(num_states);
      std_compact_tester.TestBase();
      std_compact_tester.TestExpanded();
      std_compact_tester.TestCopy();
      std_compact_tester.TestIO();
    }

    // TODO(jrosenstock): Make this work.
#if 0
    // Test with a default-constructed Fst, not a copied Fst.
    FstTester<CompactArcFst<CustomArc, CustomCompactor<CustomArc>>>
        empty_tester(/*num_states=*/0);
    const CompactArcFst<CustomArc, CustomCompactor<CustomArc>> empty_fst;
    empty_tester.TestBase(empty_fst);
    empty_tester.TestExpanded(empty_fst);
    empty_tester.TestCopy(empty_fst);
    empty_tester.TestIO(empty_fst);
#endif
  }

  // CompactFst<CustomArc, TrivialArcCompactor<CustomArc>>
  {
    for (const size_t num_states : {0, 1, 2, 3, 128}) {
      FstTester<CompactFst<CustomArc, TrivialCompactor<CustomArc>>>
          std_compact_tester(num_states);
      std_compact_tester.TestBase();
      std_compact_tester.TestExpanded();
      std_compact_tester.TestCopy();
      std_compact_tester.TestIO();
    }

    // TODO(jrosenstock): Add tests on default-constructed Fst.
  }

  // ConstFst<StdArc, uint16_t> tests
  {
    FstTester<ConstFst<StdArc, uint16_t>> std_const_tester;
    std_const_tester.TestBase();
    std_const_tester.TestExpanded();
    std_const_tester.TestCopy();
    std_const_tester.TestIO();
  }

  // CompactArcFst<StdArc, TrivialArcCompactor<StdArc>, uint16_t>
  {
    FstTester<CompactArcFst<StdArc, TrivialArcCompactor<StdArc>, uint16_t>>
        std_compact_tester;
    std_compact_tester.TestBase();
    std_compact_tester.TestExpanded();
    std_compact_tester.TestCopy();
    std_compact_tester.TestIO();
  }

  // FstTester<StdArcLookAheadFst>
  {
    FstTester<StdArcLookAheadFst> std_matcher_tester;
    std_matcher_tester.TestBase();
    std_matcher_tester.TestExpanded();
    std_matcher_tester.TestCopy();
  }

  // EditFst<StdArc> tests
  {
    FstTester<EditFst<StdArc>> std_edit_tester;
    std_edit_tester.TestBase();
    std_edit_tester.TestExpanded();
    std_edit_tester.TestAssign();
    std_edit_tester.TestCopy();
    std_edit_tester.TestMutable();
  }

  std::cout << "PASS" << std::endl;

  return 0;
}
