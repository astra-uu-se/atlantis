#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <vector>

#include "./fznTestBase.hpp"
#include "atlantis/invariantgraph/fzn/set_in.hpp"
#include "atlantis/sortedUniqueVector.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::testing {

using ::testing::AtLeast;
using ::testing::AtMost;

using namespace atlantis::invariantgraph;
using namespace atlantis::invariantgraph::fzn;

class set_inTest : public FznTestBase {
 public:
  std::string input{"input"};
  std::string set{"set"};
  std::string reified{"reified"};

  [[nodiscard]] bool isSatisfied(bool committedValue) const override {
    const auto& is = intSetVal(set);

    const bool expected = std::ranges::find(is, intVal(input, committedValue)) != is.end();
    const bool actual = boolVal(reified);

    if (isFixed(reified)) {
      const bool isSolution = violation(committedValue) == 0;
      return isSolution ? expected == actual : expected != actual;
    }
    return expected == actual;
  }

  void generate() override {
    addIntArg(input);
    addIntSetArg(set);
    constraintIdentifier = "set_in";
    generateConstraint();
  }

  [[nodiscard]] bool alwaysSatisfied() const override {
    if (!isFixed(reified)) {
      return false;
    }
    const SortedUniqueVector vals(std::vector{intSetVal(set)});
    const auto& dom = varNodeConst(input).constDomain();
    if (boolVal(reified)) {
      return dom->isContained(vals);
    }
    return dom->isDisjoint(vals);
  }

  [[nodiscard]] bool neverSatisfied() const override {
    if (!isFixed(reified)) {
      return false;
    }
    const SortedUniqueVector vals(std::vector{intSetVal(set)});
    const auto& dom = varNodeConst(input).constDomain();
    if (boolVal(reified)) {
      return dom->isDisjoint(vals);
    }
    return dom->isContained(vals);
  }

  [[nodiscard]] bool canMove() const override {
    return varId(input) != propagation::NULL_ID;
  }

  void move(bool committedValue) override {
    if (randBool()) {
      changeValue(input, committedValue);
    }
  }

  void query() override {
    if (varId(reified) != propagation::NULL_ID) {
      _solver->query(varId(reified));
    }
    _solver->query(totalViolationVarId());
  }
};

RC_GTEST_FIXTURE_PROP(set_inTest, RapidCheck, ()) { rapidCheck(); }

}  // namespace atlantis::testing