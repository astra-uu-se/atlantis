#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gen/Numeric.h>
#include <rapidcheck/gtest.h>

#include <string>
#include <vector>

#include "./fznTestBase.hpp"
#include "./tFzn_gcc.hpp"
#include "atlantis/invariantgraph/fzn/fzn_global_cardinality.hpp"
#include "atlantis/utils/domains.hpp"
#include "tFzn_gcc_count.hpp"

namespace atlantis::testing {

using ::testing::AtLeast;
using ::testing::AtMost;

using namespace atlantis::invariantgraph;
using namespace atlantis::invariantgraph::fzn;

class fzn_global_cardinalityTest : public fzn_gcc_countTest {
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
    for (const auto& input : inputs) {
      const Int val = intVal(input);
      RC_LOG() << input << " = " << val << std::endl;
      if (valToIndices.contains(val)) {
        for (const size_t index : valToIndices.at(val)) {
          RC_ASSERT(index < cover.size());
          ++counts.at(index);
        }
      }
    }
    std::vector<Int> outs;
    outs.reserve(cover.size());
    for (const auto& o : outputs) {
      const Int oVal = intVal(o, committedValue);
      RC_LOG() << o << " = " << oVal << std::endl;
      outs.emplace_back(oVal);
    }

    bool expected = duplicateOutputsEquals(committedValue);

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
      return isFixedTo(reified, true);
    }
    const auto outputDomains = getOutputDomains();

    const auto bounds = getBounds();
    Int totalLb = 0;
    Int totalUb = 0;
    bool alwaysSat = true;
    bool alwaysUnsat = false;
    for (size_t i = 0; i < bounds.size(); ++i) {
      RC_ASSERT(outputDomains.at(i).has_value());
      const auto [lb, ub] = bounds.at(i);
      totalLb += std::max(Int{0}, outputDomains.at(i)->lowerBound());
      totalUb += std::max(Int{0}, outputDomains.at(i)->upperBound());
      if (lb == ub) {
        alwaysSat &=
            outputDomains.at(i)->isFixed() && outputDomains.at(i)->contains(lb);
        alwaysUnsat |= !outputDomains.at(i)->contains(lb);
      } else {
        alwaysSat = false;
        alwaysUnsat |= ub < outputDomains.at(i)->lowerBound() ||
                       outputDomains.at(i)->upperBound() < lb;
      }
    }
    if (static_cast<Int>(inputs.size()) < totalLb ||
        totalUb < static_cast<Int>(inputs.size())) {
      return isFixedTo(reified, false);
    }
    if (isFixedTo(reified, false)) {
      return alwaysSat;
    }
    if (isFixedTo(reified, true)) {
      return alwaysSat;
    }
    return alwaysUnsat;
  }

  [[nodiscard]] bool neverSatisfied() const override {
    if (!isFixed(reified)) {
      return false;
    }
    if (inputs.empty()) {
      if (cover.empty()) {
        return isFixedTo(reified, false);
      }
      bool allFixedToZero = true;
      for (const auto& c : cover) {
        if (isFixed(c)) {
          if (!isFixedTo(c, Int{0})) {
            return isFixedTo(reified, true);
          }
          allFixedToZero &= true;
        } else {
          allFixedToZero = false;
          if (!inDomain(c, Int{0})) {
            return isFixedTo(reified, true);
          }
        }
      }
      if (allFixedToZero) {
        return isFixedTo(reified, false);
      }
    }
    if (cover.empty()) {
      return isFixedTo(reified, false);
    }
    const auto outputDomains = getOutputDomains();

    const auto bounds = getBounds();
    Int totalLb = 0;
    Int totalUb = 0;
    bool alwaysSat = true;
    bool alwaysUnsat = false;
    for (size_t i = 0; i < bounds.size(); ++i) {
      RC_ASSERT(outputDomains.at(i).has_value());
      const auto [lb, ub] = bounds.at(i);
      totalLb += std::max(Int{0}, outputDomains.at(i)->lowerBound());
      totalUb += std::max(Int{0}, outputDomains.at(i)->upperBound());
      if (lb == ub) {
        alwaysSat &=
            outputDomains.at(i)->isFixed() && outputDomains.at(i)->contains(lb);
        alwaysUnsat |= !outputDomains.at(i)->contains(lb);
      } else {
        alwaysSat = false;
        alwaysUnsat |= ub < outputDomains.at(i)->lowerBound() ||
                       outputDomains.at(i)->upperBound() < lb;
      }
    }
    if (static_cast<Int>(inputs.size()) < totalLb ||
        totalUb < static_cast<Int>(inputs.size())) {
      return isFixedTo(reified, true);
    }
    if (isFixedTo(reified, false)) {
      return alwaysSat;
    }
    return alwaysUnsat;
  }

  void generate() override {
    const size_t inputSize = true ? 1 : *rc::gen::inRange<size_t>(0, 4);
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

    addIntVarArray({IntArgState::VAR}, {{-3, 3}}, inputs, "inputs");
    addIntVarArray(std::vector(cover.size(), IntArgState::PAR),
                   {{-3, -3}, {-2, -2}, {-3, -3}}, cover, "cover");
    addIntVarArray({IntArgState::PAR, IntArgState::PAR, IntArgState::VAR},
                   {{1, 1}, {1, 1}, {-3, 3}}, outputs, "outputs");

    const bool isReified = true ? false : *rc::gen::arbitrary<bool>();
    constraintIdentifier =
        isReified ? "fzn_global_cardinality_reif" : "fzn_global_cardinality";
    if (isReified) {
      addBoolArg(reified);
    } else {
      addBoolPar(reified, true);
    }
    // Removes duplicates from cover, making testing easier:
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

RC_GTEST_FIXTURE_PROP(fzn_global_cardinalityTest, RapidCheck, ()) {
  rapidCheck(false);
}

}  // namespace atlantis::testing
