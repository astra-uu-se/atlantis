#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <algorithm>
#include <vector>

#include "./fznTestBase.hpp"
#include "atlantis/invariantgraph/fzn/int_mod.hpp"

namespace atlantis::testing {

using ::testing::AtLeast;
using ::testing::AtMost;

using namespace atlantis::invariantgraph;
using namespace atlantis::invariantgraph::fzn;

class int_modTest : public FznTestBase {
 public:
  std::string numerator{"numerator"};
  std::string denominator{"denominator"};
  std::string remainder{"quotient"};

  [[nodiscard]] bool isSatisfied(bool committedValue) const override {
    const bool expected = intVal(denominator) == 0 ? false :
        intVal(numerator) % std::abs(intVal(denominator)) == intVal(remainder);

    const bool isSolution = violation(committedValue) == 0;
    return isSolution ? expected : !expected;
  }

  void generate() override {
    addIntArg(numerator);
    addIntArg(denominator);
    addIntArg(remainder);
    constraintIdentifier = "int_mod";
    generateConstraint();
  }

  [[nodiscard]] bool alwaysSatisfied() const override {
    if (isFixedTo(denominator, Int{0})) {
      return false;
    }
    if (isFixedTo(numerator, Int{0})) {
      return isFixedTo(remainder, Int{0});
    }
    if (isFixed(numerator) && isFixed(denominator) && isFixed(remainder)) {
      return intVal(numerator) % std::abs(intVal(denominator)) == intVal(remainder);
    }
    return false;
  }

  [[nodiscard]] bool neverSatisfied() const override {
    if (isFixedTo(denominator, Int{0})) {
      return true;
    }

    if (isFixed(numerator) && isFixed(denominator) && isFixed(remainder)) {
      return intVal(numerator) % std::abs(intVal(denominator)) != intVal(remainder);
    }

    const Int lb = std::min(lowerBound(denominator), -upperBound(denominator)) + 1;
    const Int ub = std::max(upperBound(denominator), -lowerBound(denominator)) - 1;

    if (upperBound(remainder) < lb || ub < lowerBound(remainder)) {
      return true;
    }

    return false;
  }

  [[nodiscard]] bool canMove() const override {
    return std::ranges::any_of(std::array{varId(numerator), varId(denominator)}, [&](const auto vId) {
      return vId != propagation::NULL_ID;
    });
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
    if (varId(remainder) != propagation::NULL_ID) {
      _solver->query(varId(remainder));
    }
    _solver->query(totalViolationVarId());
  }
};

RC_GTEST_FIXTURE_PROP(int_modTest, RapidCheck, ()) { rapidCheck(); }

}  // namespace atlantis::testing