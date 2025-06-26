#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <algorithm>
#include <vector>

#include "./fznTestBase.hpp"
#include "atlantis/invariantgraph/fzn/int_times.hpp"

namespace atlantis::testing {

using ::testing::AtLeast;
using ::testing::AtMost;

using namespace atlantis::invariantgraph;
using namespace atlantis::invariantgraph::fzn;

class int_timesTest : public FznTestBase {
 public:
  std::string a{"a"};
  std::string b{"b"};
  std::string product{"product"};

  [[nodiscard]] bool isSatisfied(bool committedValue) const override {
    const bool expected = intVal(a) * intVal(b) == intVal(product);

    const bool isSolution = violation(committedValue) == 0;
    return isSolution ? expected : !expected;
  }

  void generate() override {
    addIntArg(a);
    addIntArg(b);
    addIntArg(product);
    constraintIdentifier = "int_times";
    generateConstraint();
  }

  [[nodiscard]] bool alwaysSatisfied() const override {
    if ((isFixedTo(a, Int{0}) || isFixedTo(b, Int{0})) && isFixedTo(product, Int {0})) {
      return true;
    }
    if (isFixed(a) && isFixed(b) && isFixed(product)) {
      return intVal(a) * intVal(b) == intVal(product);
    }
    if (isFixedTo(a, Int{1})) {
      return varNodeId(b) == varNodeId(product);
    }
    if (isFixedTo(b, Int{1})) {
      return varNodeId(a) == varNodeId(product);
    }
    return false;
  }

  [[nodiscard]] bool neverSatisfied() const override {
    if (isFixedTo(a, Int{0}) || isFixedTo(b, Int{0})) {
      return !isFixedTo(product, Int {0});
    }
    if (isFixed(a) && isFixed(b) && isFixed(product)) {
      return intVal(a) * intVal(b) != intVal(product);
    }
    return false;
  }

  [[nodiscard]] bool canMove() const override {
    return std::ranges::any_of(std::array{varId(a), varId(b)}, [&](const auto vId) {
      return vId != propagation::NULL_ID;
    });
  }

  void move(bool committedValue) override {
    if (varId(a) != propagation::NULL_ID && randBool()) {
      changeValue(a, committedValue);
    }
    if (varId(b) != propagation::NULL_ID && randBool()) {
      changeValue(b, committedValue);
    }
  }

  void query() override {
    if (varId(product) != propagation::NULL_ID) {
      _solver->query(varId(product));
    }
    _solver->query(totalViolationVarId());
  }
};

RC_GTEST_FIXTURE_PROP(int_timesTest, RapidCheck, ()) { rapidCheck(); }

}  // namespace atlantis::testing