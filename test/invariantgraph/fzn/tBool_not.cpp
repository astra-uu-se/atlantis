#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <algorithm>
#include <vector>

#include "./fznTestBase.hpp"
#include "atlantis/invariantgraph/fzn/bool_not.hpp"

namespace atlantis::testing {

using ::testing::AtLeast;
using ::testing::AtMost;

using namespace atlantis::invariantgraph;
using namespace atlantis::invariantgraph::fzn;

class bool_notTest : public FznTestBase {
 public:
  std::string input{"input"};
  std::string output{"output"};

  [[nodiscard]] bool isSatisfied(bool committedValue) const override {
    const bool expected = !boolVal(input, committedValue);
    const bool actual = boolVal(output, committedValue);

    if (isFixed(output)) {
      RC_ASSERT(totalViolationVarId() != propagation::NULL_ID);
      const bool shouldHold = violation(committedValue) == 0;
      return shouldHold ? expected == actual : expected != actual;
    }
    return actual == expected;
  }

  void generate() override {
    addBoolArg(input);
    addBoolArg(output);
    constraintIdentifier = "bool_not";
    generateConstraint();
  }

  [[nodiscard]] bool alwaysSatisfied() const override { return true; }

  [[nodiscard]] bool neverSatisfied() const override { return false; }

  [[nodiscard]] bool canMove() const override {
    return varId(input) != propagation::NULL_ID;
  }

  void move(bool committedValue) override {
    if (varId(input) != propagation::NULL_ID && randBool()) {
      changeValue(input, committedValue);
    }
  }

  void query() override {
    _solver->query(totalViolationVarId() != propagation::NULL_ID
                       ? totalViolationVarId()
                       : varId(output));
  }
};

RC_GTEST_FIXTURE_PROP(bool_notTest, RapidCheck, ()) { rapidCheck(); }

}  // namespace atlantis::testing