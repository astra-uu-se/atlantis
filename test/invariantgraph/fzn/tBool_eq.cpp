#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <algorithm>
#include <vector>

#include "./fznTestBase.hpp"
#include "atlantis/invariantgraph/fzn/bool_eq.hpp"

namespace atlantis::testing {

using ::testing::AtLeast;
using ::testing::AtMost;

using namespace atlantis::invariantgraph;
using namespace atlantis::invariantgraph::fzn;

class bool_eqTest : public FznTestBase {
 public:
  std::string a{"b_1"};
  std::string b{"b_2"};
  std::string reified{"reified"};

  [[nodiscard]] bool isSatisfied(bool committedValue) const override {
    const bool expected =
        boolVal(a, committedValue) == boolVal(b, committedValue);
    const bool actual = boolVal(reified, committedValue);

    RC_ASSERT(isFixed(reified) ==
              (totalViolationVarId() != propagation::NULL_ID));

    if (totalViolationVarId() != propagation::NULL_ID) {
      const bool isSolution = violation(committedValue) == 0;
      return isSolution ? expected == actual : expected != actual;
    }
    return expected == actual;
  }

  void generate() override {
    addBoolArg(a);
    addBoolArg(b);
    const bool isReified = *rc::gen::arbitrary<bool>();
    constraintIdentifier = isReified ? "bool_eq_reif" : "bool_eq";
    if (isReified) {
      addBoolArg(reified);
    } else {
      addBoolPar(reified, true);
    }
    generateConstraint();
  }

  [[nodiscard]] bool alwaysSatisfied() const override {
    if (isFixed(a) && isFixed(b) && isFixed(reified)) {
      return boolVal(reified) ? boolVal(a) == boolVal(b)
                              : boolVal(a) != boolVal(b);
    }
    return isFixed(a) || isFixed(b) || isFixed(reified);
  }

  [[nodiscard]] bool neverSatisfied() const override {
    if (!isFixed(reified)) {
      return false;
    }
    if (isFixed(a) && isFixed(b)) {
      return boolVal(reified) ? boolVal(a) != boolVal(b)
                              : boolVal(a) == boolVal(b);
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

RC_GTEST_FIXTURE_PROP(bool_eqTest, RapidCheck, ()) { rapidCheck(); }

}  // namespace atlantis::testing