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
  std::string a{"i_1"};
  std::string b{"i_2"};

  [[nodiscard]] bool isSatisfied(bool committedValue) const override {
    const bool expected =
        std::abs(intVal(a, committedValue)) == intVal(b, committedValue);

    const bool isSolution = violation(committedValue) == 0;
    return isSolution ? expected : !expected;
  }

  void generate() override {
    addIntArg(a);
    addIntArg(b);
    constraintIdentifier = "int_abs";
    generateConstraint();
  }

  [[nodiscard]] bool alwaysSatisfied() const override {
    if (upperBound(b) < 0) {
      return false;
    }
    if (isFixed(a)) {
      const Int aVal = intVal(a);
      if (isFixed(b)) {
        return std::abs(aVal) == intVal(b);
      }
      return false;
    }
    const auto& aDom = varNodeConst(a).constDomain();
    if (isFixed(b)) {
      const Int bVal = intVal(b);
      RC_ASSERT(!isFixed(a));
      if (aDom->size() == 2) {
        return inDomain(a, bVal) && inDomain(a, -bVal);
      }
      if (aDom->size() == 1) {
        return inDomain(a, bVal) || inDomain(a, -bVal);
      }
      return false;
    }

    const auto& bDom = varNodeConst(b).constDomain();

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
    if (upperBound(b) < 0) {
      return true;
    }
    if (isFixed(a)) {
      const Int aVal = intVal(a);
      if (isFixed(b)) {
        return std::abs(aVal) != intVal(b);
      }
      return !inDomain(b, std::abs(aVal));
    }
    if (isFixed(b)) {
      const Int bVal = intVal(b);
      RC_ASSERT(!isFixed(a));
      return !inDomain(a, bVal) && !inDomain(a, -bVal);
    }
    const auto& aDom = varNodeConst(a).constDomain();
    const auto& bDom = varNodeConst(b).constDomain();

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
    return varId(a) != propagation::NULL_ID;
  }

  void move(bool committedValue) override {
    if (varId(a) != propagation::NULL_ID && randBool()) {
      changeValue(a, committedValue);
    }
  }

  void query() override {
    _solver->query(totalViolationVarId());
  }
};

RC_GTEST_FIXTURE_PROP(int_absTest, RapidCheck, ()) { rapidCheck(); }

}  // namespace atlantis::testing