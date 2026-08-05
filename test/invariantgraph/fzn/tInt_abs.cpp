#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <algorithm>
#include <vector>

#include "./fznTestBase.hpp"
#include "atlantis/invariantgraph/fzn/int_abs.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::testing {

using ::testing::AtLeast;
using ::testing::AtMost;

using namespace atlantis::invariantgraph;
using namespace atlantis::invariantgraph::fzn;

class int_absTest : public FznTestBase {
 public:
  std::string input{"i_1"};
  std::string output{"i_2"};

  [[nodiscard]] bool isSatisfied(bool committedValue) const override {
    const bool expected =
        std::abs(intVal(input, committedValue)) == intVal(output, committedValue);

    const bool isSolution = violation(committedValue) == 0;
    return isSolution ? expected : !expected;
  }

  void generate() override {
    addIntArg(input);
    addIntArg(output);
    constraintIdentifier = "int_abs";
    generateConstraint();
    markOutputVar(output);
  }

  [[nodiscard]] bool alwaysSatisfied() const override {
    if (upperBound(output) < 0) {
      return false;
    }
    if (isFixed(input)) {
      const Int aVal = intVal(input);
      if (isFixed(output)) {
        return std::abs(aVal) == intVal(output);
      }
      return false;
    }
    const auto& aDom = varNodeConst(input).constDomain();
    if (isFixed(output)) {
      const Int bVal = intVal(output);
      RC_ASSERT(!isFixed(input));
      if (aDom->size() == 2) {
        return inDomain(input, bVal) && inDomain(input, -bVal);
      }
      if (aDom->size() == 1) {
        return inDomain(input, bVal) || inDomain(input, -bVal);
      }
      return false;
    }

    const auto& bDom = varNodeConst(output).constDomain();

    std::vector<Int> ad;
    ad.reserve(aDom->size());
    std::transform(aDom->begin(), aDom->end(), std::back_inserter(ad),
                   [&](const Int aVal) { return std::abs(aVal); });
    if ((*bDom) == SortedUniqueVector(std::move(ad))) {
      return true;
    }

    return false;
  }

  [[nodiscard]] bool neverSatisfied() const override {
    if (upperBound(output) < 0) {
      return true;
    }
    if (isFixed(input)) {
      const Int aVal = intVal(input);
      if (isFixed(output)) {
        return std::abs(aVal) != intVal(output);
      }
      return !inDomain(output, std::abs(aVal));
    }
    if (isFixed(output)) {
      const Int bVal = intVal(output);
      RC_ASSERT(!isFixed(input));
      return !inDomain(input, bVal) && !inDomain(input, -bVal);
    }
    const auto& aDom = varNodeConst(input).constDomain();
    const auto& bDom = varNodeConst(output).constDomain();

    std::vector<Int> ad;
    ad.reserve(aDom->size());
    std::transform(aDom->begin(), aDom->end(), std::back_inserter(ad),
                   [&](const Int aVal) { return std::abs(aVal); });
    if (bDom->isDisjoint(SortedUniqueVector(std::move(ad)))) {
      return true;
    }

    return false;
  }

  [[nodiscard]] bool canMove() const override {
    return varId(input) != propagation::NULL_ID;
  }

  void move(bool committedValue) override {
    if (varId(input) != propagation::NULL_ID && randBool()) {
      changeValue(input, committedValue);
    }
  }

  void query() override { _solver->query(totalViolationVarId()); }
};

RC_GTEST_FIXTURE_PROP(int_absTest, RapidCheck, ()) { rapidCheck(); }

}  // namespace atlantis::testing