#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <algorithm>
#include <vector>

#include "./fznTestBase.hpp"
#include "atlantis/invariantgraph/fzn/int_div.hpp"

namespace atlantis::testing {

using ::testing::AtLeast;
using ::testing::AtMost;

using namespace atlantis::invariantgraph;
using namespace atlantis::invariantgraph::fzn;

class int_divTest : public FznTestBase {
 public:
  std::string numerator{"numerator"};
  std::string denominator{"denominator"};
  std::string quotient{"quotient"};

  [[nodiscard]] bool isSatisfied(bool committedValue) const override {
    const bool expected =
        intVal(denominator) == 0
            ? false
            : intVal(numerator) / intVal(denominator) == intVal(quotient);

    const bool isSolution = violation(committedValue) == 0;
    return isSolution ? expected : !expected;
  }

  void generate() override {
    addIntArg(numerator);
    addIntArg(denominator);
    addIntArg(quotient);
    constraintIdentifier = "int_div";
    generateConstraint();
  }

  [[nodiscard]] bool alwaysSatisfied() const override {
    if (isFixedTo(denominator, Int{0})) {
      return false;
    }
    if (isFixedTo(numerator, Int{0})) {
      return isFixedTo(quotient, Int{0});
    }
    if (isFixedTo(quotient, Int{0})) {
      return isFixedTo(numerator, Int{0});
    }
    if (isFixedTo(denominator, Int{1})) {
      return varNodeId(numerator) == varNodeId(quotient);
    }
    if (isFixed(numerator) && isFixed(denominator) && isFixed(quotient)) {
      return intVal(numerator) / intVal(denominator) == intVal(quotient);
    }
    return false;
  }

  [[nodiscard]] bool neverSatisfied() const override {
    if (isFixedTo(denominator, Int{0})) {
      return true;
    }
    if (isFixedTo(numerator, Int{0})) {
      return !inDomain(quotient, Int{0});
    }
    if (isFixedTo(quotient, Int{0})) {
      return !inDomain(numerator, Int{0});
    }
    if (isFixed(numerator) && isFixed(denominator) && isFixed(quotient)) {
      return intVal(numerator) / intVal(denominator) != intVal(quotient);
    }
    return false;
  }

  [[nodiscard]] bool canMove() const override {
    return std::ranges::any_of(
        std::array{varId(numerator), varId(denominator)},
        [&](const auto vId) { return vId != propagation::NULL_ID; });
  }

  void move(bool committedValue) override {
    if (varId(numerator) != propagation::NULL_ID && randBool()) {
      changeValue(numerator, committedValue);
    }
    if (varId(denominator) != propagation::NULL_ID && randBool()) {
      changeValue(denominator, committedValue);
    }
  }

  void query() override {
    if (varId(quotient) != propagation::NULL_ID) {
      _solver->query(varId(quotient));
    }
    _solver->query(totalViolationVarId());
  }
};

RC_GTEST_FIXTURE_PROP(int_divTest, RapidCheck, ()) { rapidCheck(); }

}  // namespace atlantis::testing