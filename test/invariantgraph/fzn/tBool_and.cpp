#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <algorithm>
#include <vector>

#include "./fznTestBase.hpp"
#include "atlantis/invariantgraph/fzn/bool_and.hpp"

namespace atlantis::testing {

using ::testing::AtLeast;
using ::testing::AtMost;

using namespace atlantis::invariantgraph;
using namespace atlantis::invariantgraph::fzn;

class bool_andTest : public FznTestBase {
 public:
  std::string a{"a"};
  std::string b{"b"};
  std::string output{"output"};

  [[nodiscard]] bool isSatisfied(bool committedValue) const override {
    const bool expected =
        boolVal(a, committedValue) && boolVal(b, committedValue);

    if (isFixed(output) && totalViolationVarId() != propagation::NULL_ID) {
      return expected == (violation(committedValue) == 0);
    }

    return expected == boolVal(output, committedValue);
  }

  [[nodiscard]] bool alwaysSatisfied() const override {
    const bool alwaysUnsat =
        isFixedTo(a, bool{false}) || isFixedTo(b, bool{false});
    if (alwaysUnsat) {
      return inDomain(output, bool{false});
    }
    const bool alwaysSat = isFixedTo(a, bool{true}) && isFixedTo(b, bool{true});
    if (alwaysSat) {
      return inDomain(output, bool{true});
    }
    if (!isFixed(output) && isFixed(a) != isFixed(b)) {
      const VarNodeId unfixedVar = varNodeId(isFixed(a) ? b : a);
      return varNodeId(output) == unfixedVar;
    }
    return false;
  }

  [[nodiscard]] bool neverSatisfied() const override {
    if (!isFixed(output)) {
      return false;
    }
    const bool alwaysUnsat =
        isFixedTo(a, bool{false}) || isFixedTo(b, bool{false});
    if (alwaysUnsat) {
      return !inDomain(output, bool{false});
    }
    const bool alwaysSat = isFixedTo(a, bool{true}) && isFixedTo(b, bool{true});
    if (alwaysSat) {
      return !inDomain(output, bool{true});
    }
    return false;
  }

  void SetUp() override {
    FznTestBase::SetUp();
    constraintIdentifier = "bool_and";
  }

  void generate() override {
    addBoolArg(BoolArgState::PAR_FALSE, a);
    addBoolArg(BoolArgState::PAR_TRUE, b);
    addBoolArg(BoolArgState::PAR_FALSE, output);
    generateConstraint();
  }

  [[nodiscard]] bool canMove() const override {
    return varId(a) != propagation::NULL_ID || varId(b) != propagation::NULL_ID;
  }

  void move(bool committedValue) override {
    if (varId(a) != propagation::NULL_ID && randBool()) {
      changeValue(a, committedValue);
    }
    if (varId(b) != propagation::NULL_ID && randBool()) {
      changeValue(b, committedValue);
    }
  }

  void query() override { _solver->query(varId(output)); }
};

RC_GTEST_FIXTURE_PROP(bool_andTest, RapidCheck, ()) { rapidCheck(); }

}  // namespace atlantis::testing
