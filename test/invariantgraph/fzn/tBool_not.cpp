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

  [[nodiscard]] bool alwaysSatisfied() const override {
    return !neverSatisfied();
  }

  [[nodiscard]] bool neverSatisfied() const override {
    return isFixed(input) && isFixed(output) &&
           boolVal(input) == boolVal(output);
  }

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

TEST(BoolNotRegression, DefinedVarWithLiteralNegationDoesNotCrash) {
  auto model = std::make_shared<fznparser::Model>();
  auto input = std::make_shared<fznparser::BoolVar>("input");
  model->addVar(input);

  fznparser::Constraint constraint{
      "bool_not",
      std::vector<fznparser::Arg>{fznparser::BoolArg(input),
                                  fznparser::BoolArg(false)}};
  constraint.addAnnotation("defines_var",
                           fznparser::AnnotationExpression(
                               fznparser::Annotation("input")));
  model->addConstraint(std::move(constraint));

  auto graph = std::make_shared<FznInvariantGraph>(true);
  graph->open();

  EXPECT_NO_THROW(graph->build(*model));
}

}  // namespace atlantis::testing
