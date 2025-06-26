#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <algorithm>
#include <vector>

#include "./fznTestBase.hpp"
#include "atlantis/invariantgraph/fzn/int_lt.hpp"

namespace atlantis::testing {

using ::testing::AtLeast;
using ::testing::AtMost;

using namespace atlantis::invariantgraph;
using namespace atlantis::invariantgraph::fzn;

class int_ltTest : public FznTestBase {
public:
  std::string a{"i_1"};
  std::string b{"i_2"};
  std::string reified{"reified"};

  [[nodiscard]] bool isSatisfied(bool committedValue) const override {
    const bool expected =
        intVal(a, committedValue) < intVal(b, committedValue);
    const bool actual = boolVal(reified, committedValue);

    if (isFixed(reified)) {
      const bool isSolution = violation(committedValue) == 0;
      return isSolution ? expected == actual : expected != actual;
    }
    return expected == actual;
  }

  void generate() override {
    addIntArg(a);
    addIntArg(b);
    const bool isReified = *rc::gen::arbitrary<bool>();
    constraintIdentifier = isReified ? "int_lt_reif" : "int_lt";
    if (isReified) {
      addBoolArg(reified);
    } else {
      addBoolPar(reified, true);
    }
    generateConstraint();
  }

  [[nodiscard]] bool alwaysSatisfied() const override {
    if (!isFixed(reified)) {
      return false;
    }
    if (isFixedTo(reified, true)) {
      return upperBound(a) < lowerBound(b);
    }
    return lowerBound(a) >= upperBound(b);
  }

  [[nodiscard]] bool neverSatisfied() const override {
    if (!isFixed(reified)) {
      return false;
    }
    if (isFixedTo(reified, false)) {
      return upperBound(a) < lowerBound(b);
    }
    return lowerBound(a) >= upperBound(b);
  }

  [[nodiscard]] bool canMove() const override {
    return varId(a) != propagation::NULL_ID || varId(b) != propagation::NULL_ID;
  }

  void move(bool committedValue) override {
    for (const auto& input : std::array{a, b}) {
      if (varId(input) != propagation::NULL_ID && randBool()) {
        changeValue(input, committedValue);
      }
    }
  }

  void query() override {
    _solver->query(totalViolationVarId() != propagation::NULL_ID
                       ? totalViolationVarId()
                       : varId(reified));
  }
};

RC_GTEST_FIXTURE_PROP(int_ltTest, RapidCheck, ()) { rapidCheck(); }

}  // namespace atlantis::testing