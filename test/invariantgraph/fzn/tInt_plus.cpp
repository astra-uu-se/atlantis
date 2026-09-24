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

  [[nodiscard]] bool isSatisfied(const bool committedValue) const override {
    RC_LOG() << "-----" << std::endl << "int_plus::isSatisfied()";
    const Int aVal = intVal(a, committedValue);
    const Int bVal = intVal(b, committedValue);
    const Int actualSum = intVal(sum, committedValue);
    RC_LOG() << aVal << " + " << bVal << " == " << actualSum << " ("
             << (aVal + bVal) << " == " << actualSum << ')' << std::endl;
    const Int expectedSum = aVal + bVal;

    const bool inDom = inDomain(sum, expectedSum);
    const bool equals = expectedSum == actualSum;
    const bool isSolution = violation(committedValue) == 0;
    RC_ASSERT((inDom && equals) == isSolution);
    return (inDom && equals) == isSolution;
  }

  void generate() override {
    addIntArg(a);
    addIntArg(b);
    addIntArg(sum);
    constraintIdentifier = "int_plus";
    generateConstraint();
    markOutputVar(sum);
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
    return std::ranges::any_of(
        std::array{varId(a), varId(b)},
        [&](const auto vId) { return vId != propagation::NULL_ID; });
  }

  void move(const bool committedValue) override {
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