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
  std::string needle{"needle"};
  std::string output{"output"};
  std::string reified{"reified"};

  [[nodiscard]] std::pair<Int, Int> getBounds() const {
    Int lb = 0;
    Int ub = 0;
    for (const auto& input : inputs) {
      if (isFixed(input)) {
        const Int inputVal = intVal(input);
        if (isFixed(needle)) {
          if (inputVal == intVal(needle)) {
            ++lb;
            ++ub;
          }
        } else if (inDomain(needle, inputVal)) {
          ++ub;
        }
      } else if (isFixed(needle)) {
        if (inDomain(input, intVal(needle))) {
          ++ub;
        }
      } else if (!varNodeConst(input).constDomain()->isDisjoint(
                     *varNodeConst(needle).constDomain())) {
        ++ub;
      }
    }
    return {lb, ub};
  }

  [[nodiscard]] bool isSatisfied(bool committedValue) const override {
    RC_LOG() << "-----" << std::endl
             << "FznCountEqTest::isSatisfied(" << to_string(committedValue)
             << ")" << std::endl;
    Int count = 0;
    const Int n = intVal(needle);
    RC_LOG() << "needle = " << n << std::endl;
    for (const auto& input : inputs) {
      const Int i = intVal(input, committedValue);
      RC_LOG() << input << " = " << i << std::endl;
      if (i == n) {
        ++count;
      }
    }

    const Int o = intVal(output, committedValue);
    const bool expected = count == o;
    const bool actual = boolVal(reified, committedValue);

    RC_LOG() << "output = " << o << std::endl;
    RC_LOG() << "count = " << count << std::endl;
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
    const auto [lb, ub] = getBounds();
    if (lb == ub) {
      if (isFixedTo(reified, true)) {
        return isFixedTo(output, lb);
      }
      return !inDomain(output, lb);
    }
    const bool alwaysUnsat = ub < lowerBound(output) || upperBound(output) < lb;
    if (alwaysUnsat) {
      return isFixedTo(reified, bool{false});
    }
    return false;
  }

  [[nodiscard]] bool neverSatisfied() const override {
    if (!isFixed(reified)) {
      return false;
    }
    const auto [lb, ub] = getBounds();
    if (lb == ub) {
      if (isFixedTo(reified, false)) {
        return isFixedTo(output, lb);
      }
      return !inDomain(output, lb);
    }
    const bool alwaysUnsat = ub < lowerBound(output) || upperBound(output) < lb;
    if (alwaysUnsat) {
      return isFixedTo(reified, bool{true});
    }
    return false;
  }

  void generate() override {
    const size_t size = true ? 2 : *rc::gen::inRange<size_t>(0, 4);
    inputs.reserve(size);
    for (size_t i = 0; i < size; ++i) {
      inputs.emplace_back("i_" + std::to_string(i));
    }
    addIntVarArray({IntArgState::VAR, IntArgState::FIXED}, {{-1, 1}, {-1, -1}},
                   inputs);
    addIntArg(IntArgState::VAR, needle);
    addIntArg(IntArgState::PAR, 1, 1, output);

    const bool isReified = *rc::gen::arbitrary<bool>();
    constraintIdentifier =
        isReified ? "fzn_global_cardinality" : "fzn_global_cardinality_reif";
    if (isReified) {
      addBoolArg(BoolArgState::PAR_FALSE, reified);
    } else {
      addBoolPar(reified, true);
    }
    generateConstraint();
  }

  [[nodiscard]] bool canMove() const override {
    return varId(needle) != propagation::NULL_ID ||
           std::ranges::any_of(inputs, [&](const std::string& input) {
             return varId(input) != propagation::NULL_ID;
           });
  }

  void move(bool committedValue) override {
    _solver->setValue(varId(needle), -1);
    _solver->setValue(varId(inputs.at(0)), -1);
    _solver->setValue(varId(inputs.at(1)), -1);
    return;
    if (varId(needle) != propagation::NULL_ID && randBool()) {
      changeValue(needle, committedValue);
    }
    for (const auto& input : inputs) {
      if (varId(input) != propagation::NULL_ID && randBool()) {
        changeValue(input, committedValue);
      }
    }
  }

  void query() override {
    for (const auto& vId :
         std::array{varId(reified), varId(output), totalViolationVarId()}) {
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
