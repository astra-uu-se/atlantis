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
    if (!isFixed(reified)) {
      return false;
    }
    if (inputs.empty()) {
      if (cover.empty()) {
        return isFixedTo(reified, true);
      }
      bool allFixedToZero = true;
      for (const auto& c : cover) {
        if (isFixed(c)) {
          if (!isFixedTo(c, Int{0})) {
            return isFixedTo(reified, false);
          }
          allFixedToZero &= true;
        } else {
          allFixedToZero = false;
          if (!inDomain(c, Int{0})) {
            return isFixedTo(reified, false);
          }
        }
      }
      if (allFixedToZero) {
        return isFixedTo(reified, true);
      }
    }
    if (cover.empty()) {
      return isFixedTo(reified, false);
    }
    for (const auto& input : inputs) {
      bool coverContainsVal = false;
      for (const auto& c : cover) {
        if (isFixed(input)) {
          if (intVal(input) == intVal(c)) {
            coverContainsVal = true;
          }
        } else if (varNodeConst(input).constDomain()->contains(intVal(c))) {
          coverContainsVal = true;
        }
      }
      if (!coverContainsVal) {
        return isFixedTo(reified, false);
      }
    }
    const auto outputDomains = getOutputDomains();
    Int totalUb = 0;
    for (const auto& dom : outputDomains) {
      if (!dom.has_value()) {
        return isFixedTo(reified, false);
      }
      totalUb = std::max(dom->upperBound(), Int{0});
    }
    if (totalUb < static_cast<Int>(inputs.size())) {
      return isFixedTo(reified, false);
    }

    const auto bounds = getBounds();
    bool alwaysSat = true;
    for (size_t i = 0; alwaysSat && i < bounds.size(); ++i) {
      RC_ASSERT(outputDomains.at(i).has_value());
      const auto [lb, ub] = bounds.at(i);
      if (lb == ub) {
        alwaysSat &= isFixedTo(reified, true)
                         ? outputDomains.at(i)->isFixed() &&
                               outputDomains.at(i)->contains(lb)
                         : !outputDomains.at(i)->contains(lb);
      } else {
        const bool alwaysUnsat = ub < outputDomains.at(i)->lowerBound() ||
                                 outputDomains.at(i)->upperBound() < lb;
        alwaysSat &= isFixedTo(reified, false) ? alwaysUnsat : false;
      }
    }
    return alwaysSat;
  }

  [[nodiscard]] bool neverSatisfied() const override {
    if (!isFixed(reified)) {
      return false;
    }
    if (inputs.empty() && cover.empty()) {
      return isFixedTo(reified, false);
    }
    if (cover.empty()) {
      return isFixedTo(reified, true);
    }
    for (const auto& input : inputs) {
      bool coverContainsVal = false;
      for (const auto& c : cover) {
        if (isFixed(input)) {
          if (intVal(input) == intVal(c)) {
            coverContainsVal = true;
          }
        } else if (varNodeConst(input).constDomain()->contains(intVal(c))) {
          coverContainsVal = true;
        }
      }
      if (!coverContainsVal) {
        return isFixedTo(reified, true);
      }
    }

    const auto outputDomains = getOutputDomains();

    Int totalUb = 0;
    for (const auto& dom : outputDomains) {
      if (!dom.has_value()) {
        return isFixedTo(reified, true);
      }
      totalUb = std::max(dom->upperBound(), Int{0});
    }
    if (totalUb < static_cast<Int>(inputs.size())) {
      return isFixedTo(reified, true);
    }

    const auto bounds = getBounds();
    bool alwaysSat = true;
    for (size_t i = 0; alwaysSat && i < bounds.size(); ++i) {
      RC_ASSERT(outputDomains.at(i).has_value());
      const auto [lb, ub] = bounds.at(i);
      if (lb == ub) {
        const bool neverSat = isFixedTo(reified, false)
                                  ? outputDomains.at(i)->isFixed() &&
                                        outputDomains.at(i)->contains(lb)
                                  : !outputDomains.at(i)->contains(lb);
        if (neverSat) {
          return true;
        }
      } else {
        const bool alwaysUnsat = ub < outputDomains.at(i)->lowerBound() ||
                                 outputDomains.at(i)->upperBound() < lb;
        const bool neverSat = isFixedTo(reified, true) ? alwaysUnsat : false;
        if (neverSat) {
          return true;
        }
      }
    }
    return false;
  }

  void generate() override {
    const size_t inputSize = true ? 0 : *rc::gen::inRange<size_t>(0, 4);
    inputs.reserve(inputSize);
    for (size_t i = 0; i < inputSize; ++i) {
      inputs.emplace_back("i_" + std::to_string(i));
    }

    const size_t coverSize = true ? 3 : *rc::gen::inRange<size_t>(0, 4);
    cover.reserve(coverSize);
    for (size_t i = 0; i < coverSize; ++i) {
      cover.emplace_back("cover_" + std::to_string(i));
    }

    outputs.reserve(coverSize);
    for (size_t i = 0; i < coverSize; ++i) {
      outputs.emplace_back("output_" + std::to_string(i));
    }

    addIntVarArray(inputs, "inputs");
    addIntVarArray(std::vector(cover.size(), IntArgState::PAR), {{-3, -3}, {-3, -3}, {-2, -2}}, cover, "cover");
    addIntVarArray({IntArgState::VAR, IntArgState::PAR, IntArgState::VAR}, {{-3, 3}, {-3, -3}, {-3, 3}}, outputs, "outputs");

    const bool isReified = true ? true : *rc::gen::arbitrary<bool>();
    constraintIdentifier = isReified ? "fzn_global_cardinality_closed_reif"
                                     : "fzn_global_cardinality_closed";
    if (isReified) {
      addBoolArg(BoolArgState::VAR, reified);
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
  rapidCheck(false);
}

}  // namespace atlantis::testing
