#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gen/Numeric.h>
#include <rapidcheck/gtest.h>

#include <string>
#include <vector>

#include "./fznTestBase.hpp"
#include "atlantis/invariantgraph/fzn/fzn_global_cardinality.hpp"

namespace atlantis::testing {

using ::testing::AtLeast;
using ::testing::AtMost;

using namespace atlantis::invariantgraph;
using namespace atlantis::invariantgraph::fzn;

class fzn_global_cardinalityTest : public FznTestBase {
 public:
  std::vector<std::string> inputs{};
  std::vector<std::string> cover{};
  std::vector<std::string> outputs{};
  std::string reified{"reified"};

  [[nodiscard]] std::vector<std::pair<Int, Int>> getBounds() const {
    std::vector<std::pair<Int, Int>> bounds{};
    bounds.reserve(cover.size());
    for (const auto& c : cover) {
      const Int needle = intVal(c);
      Int lb = 0;
      Int ub = 0;
      for (const auto& input : inputs) {
        if (isFixed(input)) {
          if (intVal(input) == needle) {
            ++lb;
            ++ub;
          }
        } else {
          if (inDomain(input, needle)) {
            ++ub;
          }
        }
      }
      bounds.emplace_back(lb, ub);
    }
    return bounds;
  }

  [[nodiscard]] bool isSatisfied(bool committedValue) const override {
    RC_LOG() << "-----" << std::endl
             << "FznCountEqTest::isSatisfied(" << to_string(committedValue)
             << ")" << std::endl;
    std::vector<Int> counts(cover.size(), 0);
    for (size_t i = 0; i < cover.size(); ++i) {
      const Int n = intVal(cover.at(i), committedValue);
      RC_LOG() << cover.at(i) << " = " << n << std::endl;
      for (const auto& input : inputs) {
        const Int i = intVal(input, committedValue);
        RC_LOG() << input << " = " << i << std::endl;
        if (i == n) {
          ++counts.at(i);
        }
      }
    }

    std::vector<Int> outs;
    outs.reserve(cover.size());
    for (const auto& o : outputs) {
      outs.emplace_back(intVal(o, committedValue));
    }

    bool expected = true;

    for (size_t i = 0; i < cover.size(); ++i) {
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
    if (cover.empty()) {
      return isFixedTo(reified, true);
    }
    const auto bounds = getBounds();
    bool alwaysSat = true;
    for (size_t i = 0; alwaysSat && i < bounds.size(); ++i) {
      const auto [lb, ub] = bounds.at(i);
      if (lb == ub) {
        alwaysSat &= isFixedTo(reified, true) ? isFixedTo(outputs.at(i), lb) : !inDomain(outputs.at(i), lb);
      } else {
        const bool alwaysUnsat = ub < lowerBound(outputs.at(i)) || upperBound(outputs.at(i)) < lb;
        alwaysSat &= isFixedTo(reified, false) ? !alwaysUnsat : false;
      }
    }
    return alwaysSat;
  }

  [[nodiscard]] bool neverSatisfied() const override {
    if (!isFixed(reified)) {
      return false;
    }
    if (cover.empty()) {
      return isFixedTo(reified, false);
    }
    const auto bounds = getBounds();
    bool neverSat = true;
    for (size_t i = 0; neverSat && i < bounds.size(); ++i) {
      const auto [lb, ub] = bounds.at(i);
      if (lb == ub) {
        neverSat &= isFixedTo(reified, false) ? isFixedTo(outputs.at(i), lb) : !inDomain(outputs.at(i), lb);
      }
      const bool alwaysUnsat = ub < lowerBound(outputs.at(i)) || upperBound(outputs.at(i)) < lb;
      neverSat &= isFixedTo(reified, true) ? !alwaysUnsat : false;
    }
    return neverSat;
  }

  void generate() override {
    const size_t inputSize = true ? 1 : *rc::gen::inRange<size_t>(0, 4);
    inputs.reserve(inputSize);
    for (size_t i = 0; i < inputSize; ++i) {
      inputs.emplace_back("i_" + std::to_string(i));
    }

    const size_t coverSize = true ? 1 : *rc::gen::inRange<size_t>(0, 4);
    cover.reserve(coverSize);
    for (size_t i = 0; i < coverSize; ++i) {
      cover.emplace_back("cover_" + std::to_string(i));
    }

    outputs.reserve(coverSize);
    for (size_t i = 0; i < coverSize; ++i) {
      outputs.emplace_back("output_" + std::to_string(i));
    }

    addIntVarArray({IntArgState::VAR}, inputs, "inputs");
    addIntVarArray(std::vector(coverSize, IntArgState::PAR), cover, "cover");
    addIntVarArray({IntArgState::VAR}, outputs, "outputs");

    const bool isReified = false && *rc::gen::arbitrary<bool>();
    constraintIdentifier =
        isReified ? "fzn_global_cardinality_reif" : "fzn_global_cardinality";
    if (isReified) {
      addBoolArg(reified);
    } else {
      addBoolPar(reified, true);
    }
    generateConstraint();
  }

  [[nodiscard]] bool canMove() const override {
    return std::ranges::any_of(inputs, [&](const std::string& input) {
             return varId(input) != propagation::NULL_ID;
           });
  }

  void move(bool committedValue) override {
    for (const auto& input : inputs) {
      if (varId(input) != propagation::NULL_ID && randBool()) {
        changeValue(input, committedValue);
      }
    }
  }

  void query() override {
    for (const auto& output : outputs) {
      if (varId(output) != propagation::NULL_ID) {
        _solver->query(varId(output));
      }
    }
    for (const auto& vId :
         std::array{varId(reified), totalViolationVarId()}) {
      if (vId != propagation::NULL_ID) {
        _solver->query(vId);
      }
    }
  }
};

RC_GTEST_FIXTURE_PROP(fzn_global_cardinalityTest, RapidCheck, ()) {
  rapidCheck();
}

}  // namespace atlantis::testing
