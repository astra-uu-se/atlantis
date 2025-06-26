#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <algorithm>
#include <vector>

#include "./fznTestBase.hpp"
#include "atlantis/invariantgraph/fzn/int_plus.hpp"

namespace atlantis::testing {

using ::testing::AtLeast;
using ::testing::AtMost;

using namespace atlantis::invariantgraph;
using namespace atlantis::invariantgraph::fzn;

class int_plusTest : public FznTestBase {
 public:
  std::string a{"a"};
  std::string b{"b"};
  std::string sum{"sum"};

  [[nodiscard]] bool isSatisfied(bool committedValue) const override {
    RC_LOG() << "-----" << std::endl << "FznTestBase::isSatisfied()";
    const Int aVal = intVal(a);
    const Int bVal = intVal(b);
    const Int sumVal = intVal(sum);
    RC_LOG() << aVal << " + " << bVal << " == " << sumVal << " (" << (aVal + bVal) << " == " << sumVal << ')' << std::endl;
    const bool expected = aVal + bVal == sumVal;

    const bool inDom = inDomain(sum, sumVal);
    const bool isSolution = violation(committedValue) == 0;
    RC_ASSERT(inDom == isSolution);
    RC_ASSERT(expected);
    return expected;
  }

  void generate() override {
    addIntArg(IntArgState::FIXED, -1, -1, a);
    addIntArg(IntArgState::VAR, b);
    addIntArg(IntArgState::VAR, sum);
    constraintIdentifier = "int_plus";
    generateConstraint();
  }

  [[nodiscard]] bool alwaysSatisfied() const override {
    if (isFixed(a) && isFixed(b) && isFixed(sum)) {
      return intVal(a) + intVal(b) == intVal(sum);
    }
    if (isFixedTo(a, Int{0})) {
      return varNodeId(b) == varNodeId(sum);
    }
    if (isFixedTo(b, Int{0})) {
      return varNodeId(a) == varNodeId(sum);
    }
    return false;
  }

  [[nodiscard]] bool neverSatisfied() const override {
    if (isFixed(a) && isFixed(b) && isFixed(sum)) {
      return intVal(a) + intVal(b) != intVal(sum);
    }
    const Int lb = lowerBound(a) + lowerBound(b);
    const Int ub = upperBound(a) + upperBound(b);
    if (upperBound(sum) < lb || ub < lowerBound(sum)) {
      return true;
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
    if (varId(sum) != propagation::NULL_ID) {
      _solver->query(varId(sum));
    }
    _solver->query(totalViolationVarId());
  }
};

RC_GTEST_FIXTURE_PROP(int_plusTest, RapidCheck, ()) { rapidCheck(); }

}  // namespace atlantis::testing