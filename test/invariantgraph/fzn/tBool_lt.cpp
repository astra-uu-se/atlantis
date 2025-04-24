#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <algorithm>
#include <vector>

#include "./fznTestBase.hpp"
#include "atlantis/invariantgraph/fzn/bool_lt.hpp"

namespace atlantis::testing {

using ::testing::AtLeast;
using ::testing::AtMost;

using namespace atlantis::invariantgraph;
using namespace atlantis::invariantgraph::fzn;

class bool_ltTest : public FznTestBase {
 public:
  std::string a{"b_1"};
  std::string b{"b_2"};
  std::string reified{"reified"};

  [[nodiscard]] bool isSatisfied(bool committedValue) const override {
    const bool expected = (boolVal(a, committedValue) ? 1 : 0) <
                          (boolVal(b, committedValue) ? 1 : 0);
    const bool actual = boolVal(reified, committedValue);

    RC_ASSERT(isFixed(reified) ==
              (totalViolationVarId() != propagation::NULL_ID));

    if (totalViolationVarId() != propagation::NULL_ID) {
      const bool isSolution = violation(committedValue) == 0;
      return isSolution ? expected == actual : expected != actual;
    }
    return actual == expected;
  }

  void generate() override {
    addBoolArg(a);
    addBoolArg(b);
    const bool isReified = *rc::gen::arbitrary<bool>();
    constraintIdentifier = isReified ? "bool_lt_reif" : "bool_lt";
    if (isReified) {
      addBoolArg(reified);
    } else {
      addBoolPar(reified, true);
    }
    generateConstraint();
  }

  [[nodiscard]] bool alwaysSatisfied() const override {
    const size_t numFixed = std::ranges::count_if(
        std::array{a, b, reified},
        [&](const auto& identifier) { return isFixed(identifier); });
    if (numFixed == 3) {
      const bool expected = boolVal(reified);
      const bool actual = (boolVal(a) ? 1 : 0) < (boolVal(b) ? 1 : 0);
      return expected == actual;
    }
    return numFixed > 0;
  }

  [[nodiscard]] bool neverSatisfied() const override {
    const size_t numFixed = std::ranges::count_if(
        std::array{a, b, reified},
        [&](const auto& identifier) { return isFixed(identifier); });
    if (numFixed < 2) {
      return false;
    }
    if (numFixed == 3) {
      const bool expected = boolVal(reified);
      const bool actual = (boolVal(a) ? 1 : 0) < (boolVal(b) ? 1 : 0);
      return expected != actual;
    }
    if (isFixed(reified) && boolVal(reified)) {
      if (isFixed(a)) {
        return boolVal(a);
      }
      return !boolVal(b);
    }
    return false;
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

RC_GTEST_FIXTURE_PROP(bool_ltTest, RapidCheck, ()) { rapidCheck(); }

}  // namespace atlantis::testing