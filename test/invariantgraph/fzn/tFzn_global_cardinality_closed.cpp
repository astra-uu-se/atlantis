#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gen/Numeric.h>
#include <rapidcheck/gtest.h>

#include <string>
#include <vector>

#include "./fznTestBase.hpp"
#include "tFzn_count.hpp"
#include "tFzn_gcc_count.hpp"
#include "atlantis/invariantgraph/fzn/fzn_global_cardinality_closed.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::testing {

using ::testing::AtLeast;
using ::testing::AtMost;

using namespace atlantis::invariantgraph;
using namespace atlantis::invariantgraph::fzn;

class fzn_global_cardinality_closedTest : public fzn_gcc_countTest {
 public:
  [[nodiscard]] bool isSatisfied(const bool committedValue) const override {
    RC_LOG() << "-----" << std::endl
             << "FznCountEqTest::isSatisfied(" << to_string(committedValue)
             << ")" << std::endl;
    std::vector<Int> counts(cover.size(), 0);
    std::unordered_map<Int, std::vector<size_t>> valToIndices;
    valToIndices.reserve(cover.size());
    for (size_t i = 0; i < cover.size(); ++i) {
      const Int needle = intVal(cover.at(i), committedValue);
      if (valToIndices.contains(needle)) {
        valToIndices.at(needle).emplace_back(i);
      } else {
        valToIndices.emplace(needle, std::vector<size_t>{i});
      }
    }

    bool expected = true;

    for (const auto& input : inputs) {
      const Int val = intVal(input);
      RC_LOG() << input << " = " << val << std::endl;
      bool coverContainsVal = false;
      if (valToIndices.contains(val)) {
        for (const size_t index : valToIndices.at(val)) {
          RC_ASSERT(index < cover.size());
          ++counts.at(index);
          coverContainsVal = true;
        }
      }
      if (!coverContainsVal) {
        expected = false;
      }
    }

    std::vector<Int> outs;
    outs.reserve(cover.size());
    for (const auto& o : outputs) {
      const Int oVal = intVal(o, committedValue);
      RC_LOG() << o << " = " << oVal << std::endl;
      outs.emplace_back(oVal);
    }

    expected &= duplicateOutputsEquals(committedValue);

    for (size_t i = 0; i < cover.size(); ++i) {
      RC_LOG() << "counts[" << i << "] = " << counts.at(i) << std::endl;
      expected &= counts.at(i) == outs.at(i);
    }

    const bool actual = boolVal(reified, committedValue);

    RC_LOG() << "expected = " << to_string(expected) << std::endl;
    RC_LOG() << "actual = " << to_string(actual) << std::endl;

    if (isFixed(reified)) {
      const bool satAssignment = violation(committedValue) == 0;
      RC_LOG() << "satAssignment = " << to_string(satAssignment) << std::endl;
      return satAssignment ? expected == actual : expected != actual;
    }
    return expected == actual;
  }

  [[nodiscard]] bool alwaysSatisfied() const override {
    return false;
  }

  [[nodiscard]] bool neverSatisfied() const override {
    return false;
  }

  void generate() override {
    const size_t inputSize = *rc::gen::inRange<size_t>(0, 4);
    inputs.reserve(inputSize);
    for (size_t i = 0; i < inputSize; ++i) {
      inputs.emplace_back("i_" + std::to_string(i));
    }

    const size_t coverSize = *rc::gen::inRange<size_t>(0, 4);
    cover.reserve(coverSize);
    for (size_t i = 0; i < coverSize; ++i) {
      cover.emplace_back("cover_" + std::to_string(i));
    }

    outputs.reserve(coverSize);
    for (size_t i = 0; i < coverSize; ++i) {
      outputs.emplace_back("output_" + std::to_string(i));
    }

    addIntVarArray(inputs, "inputs");
    addIntVarArray(std::vector(cover.size(), IntArgState::PAR), cover, "cover");
    addIntVarArray(outputs, "outputs");

    const bool isReified = *rc::gen::arbitrary<bool>();
    constraintIdentifier = isReified ? "fzn_global_cardinality_closed_reif"
                                     : "fzn_global_cardinality_closed";
    if (isReified) {
      addBoolArg(reified);
    } else {
      addBoolPar(reified, true);
    }
    fixGenerate();
    generateConstraint();
  }

  void query() override {
    for (const auto& output : outputs) {
      if (varId(output) != propagation::NULL_ID) {
        _solver->query(varId(output));
      }
    }
    for (const auto& vId : std::array{varId(reified), totalViolationVarId()}) {
      if (vId != propagation::NULL_ID) {
        _solver->query(vId);
      }
    }
  }
};

RC_GTEST_FIXTURE_PROP(fzn_global_cardinality_closedTest, RapidCheck, ()) {
  rapidCheck(true, true);
}

}  // namespace atlantis::testing
